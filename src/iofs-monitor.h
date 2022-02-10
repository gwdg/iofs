#include <stdint.h>
#include <time.h>

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
  char * in_server_port;
  char * in_db;
  int interval;
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

void monitor_init(monitor_options_t * options);
void monitor_finalize();

/**
 *
 */
void monitor_start_activity(monitor_activity_t* activity);
void monitor_end_activity(monitor_activity_t* activity, monitor_counter_t * counter, uint64_t value);
