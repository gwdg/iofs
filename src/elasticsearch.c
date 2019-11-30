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

#include <iofs-monitor.h>


typedef struct{
  // mutex could be here
  // buffer could be here
  int count;
  uint64_t value;
  double latency;
} monitor_counter_internal_t;

typedef struct {
  int started;
  pthread_t reporting_thread;
  FILE * logfile;
  int es_fd; // es file descriptor

  int timestep;
  monitor_counter_internal_t value[2][COUNTER_LAST]; // two timesteps
} monitor_internal_t;

static monitor_options_t options;
static monitor_internal_t monitor;

monitor_counter_t counter[COUNTER_LAST] = {
  {"getattr", COUNTER_GETATTR},
  {"access", COUNTER_ACCESS},
  {"read", COUNTER_READ},
  {"read buffer", COUNTER_READ_BUF}
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

static void* reporting_thread(void * user){
  char json[1024*1024];
  while(monitor.started){
    sleep(options.interval);
    char * ptr = json;
    monitor.timestep = (monitor.timestep + 1) % 2;

    ptr += sprintf(ptr, "{");

    for(int i=0; i < COUNTER_LAST; i++){
      monitor_counter_internal_t * p = & monitor.value[monitor.timestep][i];
      double mean_latency = p->latency / p->value;
      if (options.verbosity > 3){
        fprintf(monitor.logfile, "%s: %d %"PRIu64" %e\n", counter[i].name, p->count, p->value, mean_latency);
      }
      if( options.es_server ){
        if(i > 0) ptr += sprintf(ptr, ",");
        ptr += sprintf(ptr, "\"%s\":%d",  counter[i].name, p->count);
      }
      p->count = 0;
      p->value = 0;
      p->latency = 0;
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

void monitor_end_activity(monitor_activity_t* activity, monitor_counter_t * counter, uint64_t count){
  clock_t t_end;
  t_end = clock();
  double t = ((double) (t_end - activity->t_start)) / CLOCKS_PER_SEC;
  int ts = monitor.timestep;
  int type = counter->type;
  monitor.value[ts][type].value += count;
  monitor.value[ts][type].latency += t;
  monitor.value[ts][type].count++;
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