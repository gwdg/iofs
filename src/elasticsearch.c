// https://curl.haxx.se/libcurl/c/libcurl-tutorial.html
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>

#include <iofs-monitor.h>

enum hist_type_t{
  HIST_READ,
  HIST_WRITE,
  HIST_LAST
};

static char * hist_names[HIST_LAST] = {"read", "write"};
#define HIST_BUCKETS 7
static int hist_sizes[HIST_BUCKETS - 1] = {4096, 4096*2, 4096*4, 65536, 65536*2, 65536*4};

typedef struct{
  // mutex could be here
  // buffer could be here
  int count;
  uint64_t value;
  double latency;
  float latency_max;
  float latency_min;
} monitor_counter_internal_t;

typedef struct{
  // intervals: 4, 8, 16, 32, 64, 128, 256, 1024+
  monitor_counter_internal_t interval[HIST_BUCKETS];
} monitor_histogram_t;

typedef struct {
  int started;
  pthread_t reporting_thread;
  FILE * logfile;
  int es_fd; // es file descriptor

  int timestep;
  monitor_counter_internal_t  value[2][COUNTER_LAST]; // two timesteps
  monitor_histogram_t         hist[2][HIST_LAST];
} monitor_internal_t;

static monitor_options_t options;
static monitor_internal_t monitor;

monitor_counter_t counter[COUNTER_LAST] = {
  {"MD get", COUNTER_MD_GET, COUNTER_NONE},
  {"MD mod", COUNTER_MD_MOD, COUNTER_NONE},
  {"MD other", COUNTER_MD_OTHER, COUNTER_NONE},
  {"read", COUNTER_READ, COUNTER_NONE},
  {"write", COUNTER_WRITE, COUNTER_NONE},

  {"getattr", COUNTER_GETATTR, COUNTER_MD_GET},
  {"access", COUNTER_ACCESS, COUNTER_MD_GET},
  {"readlink", COUNTER_READLINK, COUNTER_MD_GET},
  {"opendir", COUNTER_OPENDIR, COUNTER_MD_GET},
  {"readdir", COUNTER_READDIR, COUNTER_MD_GET},
  {"releasedir", COUNTER_RELEASEDIR, COUNTER_NONE},
  {"mkdir", COUNTER_MKDIR, COUNTER_MD_MOD},
  {"symlink", COUNTER_SYMLINK, COUNTER_MD_MOD},
  {"unlink", COUNTER_UNLINK, COUNTER_MD_MOD},
  {"rmdir", COUNTER_RMDIR, COUNTER_MD_MOD},
  {"rename", COUNTER_RENAME, COUNTER_MD_MOD},
  {"link", COUNTER_LINK, COUNTER_MD_MOD},
  {"chmod", COUNTER_CHMOD, COUNTER_MD_MOD},
  {"chown", COUNTER_CHOWN, COUNTER_MD_MOD},
  {"truncate", COUNTER_TRUNCATE, COUNTER_MD_MOD},
  {"utimens", COUNTER_UTIMENS, COUNTER_MD_MOD},
  {"create", COUNTER_CREATE, COUNTER_MD_MOD},
  {"open", COUNTER_OPEN, COUNTER_MD_GET},
  {"read_buf", COUNTER_READ_BUF, COUNTER_NONE},
  {"write_buf", COUNTER_WRITE_BUF, COUNTER_NONE},
  {"statfs", COUNTER_STATFS, COUNTER_MD_OTHER},
  {"flush", COUNTER_FLUSH, COUNTER_NONE},
  {"release", COUNTER_RELEASE, COUNTER_NONE},
  {"fsync", COUNTER_FSYNC, COUNTER_NONE},
  {"fallocate", COUNTER_FALLOCATE, COUNTER_NONE},
  {"setxattr", COUNTER_SETXATTR, COUNTER_MD_MOD},
  {"getxattr", COUNTER_GETXATTR, COUNTER_MD_GET},
  {"listxattr", COUNTER_LISTXATTR, COUNTER_MD_GET},
  {"removexattr", COUNTER_REMOVEXATTR, COUNTER_MD_MOD},
  {"lock", COUNTER_LOCK, COUNTER_NONE},
  {"flock", COUNTER_FLOCK, COUNTER_NONE},
  };

static void es_send(char* data, int len){
  // send the data to the server
  ssize_t pos = 0;
  while(pos != len){
    ssize_t ret = write(monitor.es_fd, & data[pos], len - pos);
    if ( ret == -1 ){
      monitor.es_fd = 0;
      fprintf(monitor.logfile, "error during sending to es: %s", strerror(errno));
      break;
    }
    pos += ret;
  }
}

static void submit_to_es(char * json, int json_len){
  if(! options.es_server ) return;

  if (monitor.es_fd == 0){
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    struct addrinfo* res=0;
    int err=getaddrinfo(options.es_server, options.es_server_port, &hints, &res);
    if (err!=0) {
      fprintf(monitor.logfile, "failed to resolve remote socket address (err=%d)" ,err);
    }

    int sockfd;
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        fprintf(monitor.logfile, "Socket creation failed: %s\n", strerror(errno));
    }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
        fprintf(monitor.logfile, "Connection with the server %s:%s failed:  %s\n", options.es_server, options.es_server_port, strerror(errno));
        return;
    }
    monitor.es_fd = sockfd;
  }

  // send the header to the server
  char buff[1024];
  int len = sprintf(buff, "POST /%s HTTP/1.1\nHost: \"%s\"\nContent-Length: %d\nContent-Type: application/x-www-form-urlencoded\n\n", options.es_uri, options.es_server, json_len);
  es_send(buff, len);
  es_send(json, json_len);
}

static inline void clean_value(monitor_counter_internal_t * p){
  p->count = 0;
  p->value = 0;
  p->latency = 0;
  p->latency_min = INFINITY;
  p->latency_max = 0;
}

static void* reporting_thread(void * user){
  char json[1024*1024];
  int lastCounter;
  if (options.detailed_logging){
    lastCounter = COUNTER_LAST;
  }else{
    lastCounter = COUNTER_WRITE + 1;
  }
  while(monitor.started){
    sleep(options.interval);
    char * ptr = json;
    monitor.timestep = (monitor.timestep + 1) % 2;

    ptr += sprintf(ptr, "{");

    // print histograms
    for(int i=0; i < HIST_LAST; i++){
      monitor_counter_internal_t * p;
      for(int j = 0; j < HIST_BUCKETS - 1; j++){
        p = & monitor.hist[monitor.timestep][i].interval[j];
        printf("%s_%dk: %d\n", hist_names[i], hist_sizes[j]/1024, p->count);
        clean_value(p);
      }
      p = & monitor.hist[monitor.timestep][i].interval[HIST_BUCKETS-1];
      printf("%s_large: %d\n", hist_names[i], p->count);
      clean_value(p);
    }
    //

    for(int i=0; i < lastCounter; i++){
      monitor_counter_internal_t * p = & monitor.value[monitor.timestep][i];
      double mean_latency = p->latency / p->value;
      if (options.verbosity > 3){
        fprintf(monitor.logfile, "%s: %d %"PRIu64" %e %e %e\n", counter[i].name, p->count, p->value, mean_latency, p->latency_min, p->latency_max);
      }
      if( options.es_server ){
        if(i > 0) ptr += sprintf(ptr, ",");
        ptr += sprintf(ptr, "\"%s\":%d",  counter[i].name, p->count);
        ptr += sprintf(ptr, ", \"%s_l\":%e",  counter[i].name, p->latency);
        ptr += sprintf(ptr, ", \"%s_lmin\":%e",  counter[i].name, p->latency_min);
        ptr += sprintf(ptr, ", \"%s_lmax\":%e",  counter[i].name, p->latency_max);
      }
      clean_value(p);
    }
    if (options.verbosity > 5){
      fflush(monitor.logfile);
    }
    ptr += sprintf(ptr, "}\n");
    submit_to_es(json, (int)(ptr - json));
  }
  return NULL;
}

void monitor_start_activity(monitor_activity_t* activity){
  activity->t_start = clock();
}


static inline void update_counter(monitor_counter_internal_t * counter, double time, uint64_t count){
  // On some machine may be not thread safe and lead to some inaccuracy, we accept this for performance
  counter->count++;
  counter->value += count;
  counter->latency += time;
  if(time < counter->latency_min){
    counter->latency_min = time;
  }
  if(time > counter->latency_max){
    counter->latency_max = time;
  }
}

static void inline update_hist(enum hist_type_t type, int ts, double t, uint64_t size){
  int i;
  for(i = 0; i < HIST_BUCKETS - 1; i++){
    if(size < hist_sizes[i]){
      break;
    }
  }
  update_counter(& monitor.hist[ts][type].interval[i], t, size);
}


void monitor_end_activity(monitor_activity_t* activity, monitor_counter_t * counter, uint64_t count){
  clock_t t_end;
  t_end = clock();
  double t = ((double) (t_end - activity->t_start)) / CLOCKS_PER_SEC;

  int ts = monitor.timestep;
  update_counter(& monitor.value[ts][counter->type], t, count);

  if(counter->parent_type != COUNTER_NONE){
    update_counter(& monitor.value[ts][counter->parent_type], t, count);
  }
  if(counter->type == COUNTER_READ){
    update_hist(HIST_READ, ts, t, count);
  }else if (counter->type == COUNTER_WRITE ){
    update_hist(HIST_WRITE, ts, t, count);
  }
}

void monitor_init(monitor_options_t * o){
  memcpy(& options, o, sizeof(options));
  memset(& monitor, 0, sizeof(monitor));
  monitor.logfile = fopen(o->logfile, "w+");
  if(! monitor.logfile) monitor.logfile = fopen(o->logfile, "w");
  if(! monitor.logfile) monitor.logfile = stderr;
  memset(monitor.value, 0, sizeof(monitor.value));

  monitor.started = 1;
  if(pthread_create(& monitor.reporting_thread, NULL, reporting_thread, NULL)){
    fprintf(monitor.logfile, "Error couldn't create background reporting thread\n");
    exit(1);
  }
}

void monitor_finalize(){
  monitor.started = 0;
  if(pthread_join(monitor.reporting_thread, NULL)) {
    fprintf(monitor.logfile, "Error joining background thread\n");
  }
  fclose(monitor.logfile);
}
