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
#define IN_PORT 0x104
#define IN_DB 0x105

static error_t parse_opt(int , char *, struct argp_state *);

const char *argp_program_version = "iofs 0.8";

const char *argp_program_bug_address = "<hpc-support@gwdg.de>";

/* This structure is used by main to communicate with parse_opt. */
typedef struct cli_arguments_t
{
  char *args[2];            /* ARG1 and ARG2 */
  char *output;
  char *outfile;            /* Argument for -o */
  char *logfile;            /* Argument for -o */
  char *es_server, *es_server_port, *es_uri;
  char *in_server, *in_server_port, *in_db;
  int verbosity;
  int detailed_logging;
  int interval;
} cli_arguments_t;

static cli_arguments_t arguments;

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option arg_options[] =
{
  {"verbosity", 'v', 0, 0, "Produce verbose output"},
  {"interval", 'i', 0, 0, "output interval"},
  {"output", 'o', "F", 0, "which output to use?"},
  {"logfile",  'l', "LOGFILE", 0, "location of logs"},
  {"outfile",  'O', "/tmp/iofs.out", 0, "location of data"},
  {"es-server", ES_SERVER, "http://localhost", 0, "Location of the elasticsearch server"},
  {"es-port", ES_PORT, "8086", 0, "Elasticsearch Port"},
  {"es-uri", ES_URI, "no clue", 0, "something"},
  {"in-server", IN_SERVER, "http://localhost", 0, "Location of the influxdb server"},
  {"in-port", IN_PORT, "8086", 0, "InfluxDB Port"},
  {"in-db", IN_DB, "moep", 0, "database name"},
  {0}
};


static char args_doc[] = "fuse-mountpont source-directory";
static char doc[] =
"IOFS -- The I/O file system - A FUSE file system developed for I/O monitoring";
static struct argp argp = {arg_options, parse_opt, args_doc, doc};
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  cli_arguments_t *arguments = state->input;

  switch (key)
    {
    case 'v':
      arguments->verbosity = 10;
      break;
    case 'i':
      arguments->interval = 1;
      break;
    case 'l':
      arguments->logfile = arg;
      break;
    case 'o':
      arguments->output = arg;
      break;
    case 'O':
      arguments->outfile = arg;
      break;
    case ES_SERVER:
      arguments->es_server = arg;
      break;
    case ES_PORT:
      arguments->es_server_port = arg;
      break;
    case ES_URI:
      arguments->es_uri = arg;
      break;
    case IN_SERVER:
      arguments->in_server = arg;
      break;
    case IN_PORT:
      arguments->in_server_port = arg;
      break;
    case IN_DB:
      arguments->in_db = arg;
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
