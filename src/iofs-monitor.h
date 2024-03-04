#ifndef IOFS_MONITOR_H
#define IOFS_MONITOR_H

#include <stdint.h>
#include <string.h>
#include <time.h>

// TODO: MOVE ME
typedef enum classification_type_t {
  CLASSIFICATION_RANDOM_UNCACHED,
  CLASSIFICATION_REVERSE,
  CLASSIFICATION_SAME_OFFSET,
  CLASSIFICATION_SEQUENTIAL,
  CLASSIFICATION_UNCLASSIFIED,
  CLASSIFICATION_LAST
} classification_type_t;
/* All classifications are either constant or linear polynomials */
typedef struct classification_t {
  classification_type_t benchmark_type;
  int is_read_op;
  double slope;
  double y_intercept;
  // NULL == 0 is never a valid bound, because we can't have negative access sizes.
  // Thus y = 0 <=> [x,y] = [x, \inf)
  double left_bound;
  double right_bound;
} classification_t;

static struct {
  classification_type_t key;
  char *val
} classification_lookup[] = {
  {CLASSIFICATION_RANDOM_UNCACHED, "RandomUncached"},
  {CLASSIFICATION_REVERSE, "Reverse"},
  {CLASSIFICATION_SAME_OFFSET, "SameOffset"},
  {CLASSIFICATION_SEQUENTIAL, "Sequential"}
};

static int str_to_classification_type(const char *s, classification_type_t *out) {
  for (int i=0; i < sizeof(classification_lookup) / sizeof(classification_lookup[0]); ++i) {
    if (!strcmp(s, classification_lookup[i].val)) {
      // Found it!
      *out = classification_lookup[i].key;
      return 1;
    }
  }
  // unparsable...
  return 0;
}
static int classification_type_to_str(const classification_type_t c, char *out) {
  for (int i=0; i < sizeof(classification_lookup) / sizeof(classification_lookup[0]); ++i) {
    if (c == classification_lookup[i].key) {
      // Found it!
      out = classification_lookup[i].val;
      return 1;
    }
  }
  // unparsable...
  return 0;
}

enum counter_type_t{
  COUNTER_MD_GET,
  COUNTER_MD_MOD,
  COUNTER_MD_OTHER,
  COUNTER_READ,
  COUNTER_WRITE, // stop here for the short report
  COUNTER_GETATTR,
  COUNTER_ACCESS,
  COUNTER_READLINK,
  COUNTER_OPENDIR,
  COUNTER_READDIR,
  COUNTER_RELEASEDIR,
  COUNTER_MKDIR,
  COUNTER_SYMLINK,
  COUNTER_UNLINK,
  COUNTER_RMDIR,
  COUNTER_RENAME,
  COUNTER_LINK,
  COUNTER_CHMOD,
  COUNTER_CHOWN,
  COUNTER_TRUNCATE,
  COUNTER_UTIMENS,
  COUNTER_CREATE,
  COUNTER_OPEN,
  COUNTER_READ_BUF,
  COUNTER_WRITE_BUF,
  COUNTER_STATFS,
  COUNTER_FLUSH,
  COUNTER_RELEASE,
  COUNTER_FSYNC,
  COUNTER_FALLOCATE,
  COUNTER_SETXATTR,
  COUNTER_GETXATTR,
  COUNTER_LISTXATTR,
  COUNTER_REMOVEXATTR,
  COUNTER_LOCK,
  COUNTER_FLOCK,
  COUNTER_LAST,
  COUNTER_NONE
};

typedef struct {
  char * logfile;
  char * outfile;
  int detailed_logging;
  int verbosity;
  char * es_server;
  char * es_server_port;
  char * es_uri;
  char * in_server;
  char * in_db;
  char * in_username;
  char * in_password;
  char * in_tags;
  int interval;
  classification_t **classifications;
} monitor_options_t;

struct monitor_counter_t{
  char const * name;
  enum counter_type_t type;
  enum counter_type_t parent_type; // the additional parent/meta type (e.g., MD_READ)
};


typedef struct monitor_counter_t monitor_counter_t;
extern monitor_counter_t counter[];

struct monitor_activity_t{
  clock_t t_start;
};

typedef struct monitor_activity_t monitor_activity_t;

// CONTIANED IN elasticsearch.c
void monitor_init(monitor_options_t * options);
void monitor_finalize();

void monitor_start_activity(monitor_activity_t* activity);
void monitor_end_activity(monitor_activity_t* activity, monitor_counter_t * counter, uint64_t value);
#endif
