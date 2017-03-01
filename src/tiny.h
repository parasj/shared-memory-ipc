#ifndef TINYFILE_TINY
#define TINYFILE_TINY

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/queue.h>

#define MSGQFILE "/tmp/.tiny_msgqfile"
#define CLIENT_MSGQFILE_FMT "/tmp/.tiny_msgqfile.%d"

typedef enum msg_t {
  MSG_INIT_REQUEST_TYPE = 1,
  MSG_INIT_RESPONSE_TYPE,
  MSG_CMP_TYPE,
  MSG_UNCMP_TYPE,
  MSG_FIN_TYPE
} msg_t;

typedef struct tiny_msgbuf {
  long mtype;
  union tiny_args {
    struct initialize {
      int client_key;
    } initialize;
    struct finish {
      int client_key;
    } finish;
    struct compress_args {
      char *input;
      size_t input_length;
      char *compressed;
      size_t *compressed_length;
    } compress_args;
    struct uncompress_args {
      char *compressed;
      size_t length;
      char *uncompressed;
    } uncompress_args;
  } msgdata;
} tiny_msgbuf;

LIST_HEAD(tiny_client_list, tiny_client);

typedef struct tiny_client {
  int client_number;
  int client_key;
  int client_msgqid;
  LIST_ENTRY(tiny_client) next_client;
} tiny_client;

#endif
