#ifndef IOFS_MONITOR_H
#define IOFS_MONITOR_H

#include <stdint.h>
#include <time.h>

enum counter_type_t{
  COUNTER_GETATTR,
  COUNTER_ACCESS,
  COUNTER_READ,
  COUNTER_READ_BUF,
  COUNTER_LAST
};

typedef struct {
  char * logfile;
  int verbosity;
  char * elasticsearch_server;
  char * elasticsearch_server_port;
  int interval;
} monitor_options_t;

struct monitor_counter_t{
  char const * name;
  enum counter_type_t type;
};

typedef struct monitor_counter_t monitor_counter_t;
monitor_counter_t counter[COUNTER_LAST];

struct monitor_activity_t{
  clock_t t_start;
};

typedef struct monitor_activity_t monitor_activity_t;

void monitor_init(monitor_options_t * options);
void monitor_finalize();

/**
 *
 */
void monitor_start_activity(monitor_activity_t* activity);
void monitor_end_activity(monitor_activity_t* activity, monitor_counter_t * counter, uint64_t value);

#endif
