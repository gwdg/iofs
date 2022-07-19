/* parsing the command line options for iofs.c
 * might later include some config file handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>

#define ES_SERVER 0x100
#define ES_PORT 0x101
#define ES_URI 0x102
#define IN_SERVER 0x103
#define IN_DB 0x104
#define CLASSIFICATION_FILE 0x105

#define BUF_LEN 256

static error_t parse_opt(int , char *, struct argp_state *);

const char *argp_program_version = "iofs 0.8";

const char *argp_program_bug_address = "<hpc-support@gwdg.de>";

/* This structure is used by main to communicate with parse_opt. */
/**
 * @brief 
 * 
 */
typedef struct options_t
{
  char *args[2];            /* ARG1 and ARG2 */
  char outfile[BUF_LEN];            /* Argument for -o */
  char logfile[BUF_LEN];            /* Argument for -o */
  char es_server[BUF_LEN], es_server_port[6], es_uri[BUF_LEN];
  char in_server[BUF_LEN], in_db[BUF_LEN], in_username[BUF_LEN], in_password[BUF_LEN], in_tags[BUF_LEN];
  int verbosity;
  int detailed_logging;
  int interval;
  char classificationfile[BUF_LEN];
} options_t;

static int append_tags(options_t *, char *);

/**
 * @brief 
 * 
 */
static options_t arguments = {
  .outfile = "/tmp/iofs.out",
  .logfile = "/tmp/iofs.log",
  .verbosity = 10,
  .detailed_logging = 1,
  .interval = 1,
  .es_server = "",
  .in_server = "",
  .in_username = "",
  .in_server = "",
  .classificationfile = ""
};

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option arg_options[] = {
  {"verbosity", 'v', "10", 0, "Produce verbose output"},
  {"interval", 'i', "1", 0, "output interval in seconds"},
  {"logfile",  'l', "/tmp/iofs.log", 0, "location of logs"},
  {"outfile",  'O', "/tmp/iofs.out", 0, "location of data"},
  {"es-server", ES_SERVER, "http://localhost", 0, "Location of the elasticsearch server"},
  {"es-port", ES_PORT, "8086", 0, "Elasticsearch Port"},
  {"es-uri", ES_URI, "no clue", 0, "something"},
  {"in-server", IN_SERVER, "http://localhost:8086", 0, "Location of the influxdb server with port"},
  {"in-db", IN_DB, "moep", 0, "database name"},
  {"in-tags", 't', "cluster=hpc-1", 0, "Custom tags for InfluxDB"},
  {"classificationfile", CLASSIFICATION_FILE, "/path/to/model.csv", 0, "Expected performance model. See documentation for more."},
  {0}
};


static char args_doc[] = "fuse-mountpont source-directory";
static char doc[] =
"IOFS -- The I/O file system - A FUSE file system developed for I/O monitoring";
static struct argp argp = {arg_options, parse_opt, args_doc, doc};

/**
 * @brief 
 * 
 * @param key 
 * @param arg 
 * @param state 
 * @return error_t 
 */
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  options_t *arguments = state->input;

  switch (key)
    {
    case 'v':
      arguments->verbosity = atoi(arg);
      break;
    case 'i':
      arguments->interval = atoi(arg);
      break;
    case 'l':
      if (snprintf(arguments->logfile, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case 'O':
      if (snprintf(arguments->outfile, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case ES_SERVER:
      if (snprintf(arguments->es_server, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case ES_PORT:
      if (snprintf(arguments->es_server_port, 6, "%s", arg) > 6) {
        printf("Input argument %s bigger then %d. Aborting", key, 6);
        return 1;
      }
      break;
    case ES_URI:
      if (snprintf(arguments->es_uri, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case IN_SERVER:
      if (snprintf(arguments->in_server, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case IN_DB:
      if (snprintf(arguments->in_db, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case CLASSIFICATION_FILE:
      if (snprintf(arguments->classificationfile, BUF_LEN, "%s", arg) > BUF_LEN) {
        printf("Input argument %s bigger then %d. Aborting", key, BUF_LEN);
        return 1;
      }
      break;
    case 't': ;
      //TODO: Do something if this fails
      int ret = append_tags(arguments, arg);
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 2)
        /* Too many arguments. */
        argp_usage (state);

      arguments->args[state->arg_num] = arg;
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 2)
        /* Not enough arguments. */
        argp_usage (state);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


// Parse the buffer for config info. Return an error code or 0 for no error.
// TODO Check how resistent this is to whitespaces
/**
 * @brief 
 * 
 * @param buf 
 * @param arguments 
 * @return int 
 */
static int parse_config(char *buf, options_t *arguments) {
  char dummy[BUF_LEN];
//  printf(buf);
  if (sscanf(buf, " %s", dummy) == EOF) return 0; // blank line
  if (sscanf(buf, " %[#]", dummy) == 1) return 0; // comment
  if (sscanf(buf, " influxdb-address = %s", arguments->in_server) == 1) return 0;
  if (sscanf(buf, " influxdb-database = %s", arguments->in_db) == 1) return 0;
  if (sscanf(buf, " influxdb-username = %s", arguments->in_username) == 1) return 0;
  if (sscanf(buf, " influxdb-password = %s", arguments->in_password) == 1) return 0;
  if (sscanf(buf, " influxdb-tags = %s", dummy) == 1) return append_tags(arguments, dummy);
  if (sscanf(buf, " elasticsearch-address = %s", arguments->es_server) == 1) return 0;
  if (sscanf(buf, " elasticsearch-port = %s", arguments->es_server_port) == 1) return 0;
  if (sscanf(buf, " elasticsearch-uri = %s", arguments->es_uri) == 1) return 0;
  if (sscanf(buf, " logfile = %s", arguments->logfile) == 1) return 0;
  if (sscanf(buf, " outfile = %s", arguments->outfile) == 1) return 0;
  if (sscanf(buf, " interval = %lu", arguments->interval) == 1) return 0;
  if (sscanf(buf, " verbosity = %lu", arguments->verbosity) == 1) return 0;
  if (sscanf(buf, " classificationfile = %s", arguments->classificationfile) == 1) return 0;
  return 3; // syntax error
}

// TODO document what tags are (a influx concept)
/**
 * @brief 
 * 
 * @param arguments 
 * @param tags 
 * @return int 
 */
static int append_tags(options_t *arguments, char *tags) {
  if (snprintf(
        arguments->in_tags + strlen(arguments->in_tags),
        BUF_LEN + strlen(arguments->in_tags), ",%s", tags)
      > BUF_LEN) {
    printf("Could not add tag %s, too many tags", tags);
    return 5;
  }
  return 0;
}

/**
 * @brief 
 * 
 * @param config_path 
 * @param arguments 
 * @return int 
 */
int read_config (char * config_path, options_t *arguments) {
  FILE *f = fopen(config_path, "r");
  if (! f) return 1;
  char buf[BUF_LEN];
  int line_number = 0;
  while (fgets(buf, sizeof buf, f)) {
    ++line_number;
    int err = parse_config(buf, arguments);
    if (err) fprintf(stderr, "error line %d: %d\n", line_number, err);
  }
  return 0;
}
