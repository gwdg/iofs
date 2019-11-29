// https://curl.haxx.se/libcurl/c/libcurl-tutorial.html
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <iofs-monitor.h>


typedef struct{
  int count;
  uint64_t value;
  double latency;
} monitor_counter_internal_t;

typedef struct {
  int started;
  pthread_t reporting_thread;
  FILE * logfile;
  monitor_counter_internal_t value[COUNTER_LAST];
} monitor_internal_t;

static monitor_internal_t monitor;

monitor_counter_t counter[COUNTER_LAST] = {
  {"getattr", COUNTER_GETATTR},
  {"access", COUNTER_ACCESS},
  {"read", COUNTER_READ},
  {"read buffer", COUNTER_READ_BUF}
  };

static void* reporting_thread(void * user){
  while(monitor.started){
    sleep(1);
    for(int i=0; i < COUNTER_LAST; i++){
      monitor_counter_internal_t * p = & monitor.value[i];
      double mean_latency = p->latency / p->value;
      fprintf(monitor.logfile, "%s: %d %"PRIu64" %e\n", counter[i].name, p->count, p->value, mean_latency);
    }
    fflush(monitor.logfile);
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
  int type = counter->type;
  monitor.value[type].value += count;
  monitor.value[type].latency += t;
  monitor.value[type].count++;
}

void monitor_init(){
  monitor.logfile = fopen("iofs.log", "w+");
  if(! monitor.logfile) monitor.logfile = fopen("iofs.log", "w");
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
