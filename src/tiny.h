#ifndef TINYFILE_TINY
#define TINYFILE_TINY

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/queue.h>
#include <sys/time.h>

#define MSGQFILE "/tmp/.tiny_msgqfile"
#define CLIENT_MSGQFILE_FMT "/tmp/.tiny_msgqfile.%d"

#define start() struct timeval ts;gettimeofday(&ts, NULL)
#define end()   struct timeval te;gettimeofday(&te, NULL);double startTime=ts.tv_sec+ts.tv_usec*1e-6;double endTime = te.tv_sec+te.tv_usec*1e-6
#define TIME    endTime - startTime

typedef enum msg_t {
  MSG_INIT_REQUEST_TYPE = 1,
  MSG_FIN_TYPE,
  MSG_INIT_RESPONSE_TYPE,
  MSG_CMP_TYPE,
  MSG_UNCMP_TYPE
} msg_t;

typedef struct tiny_msgbuf {
  long mtype;
  union tiny_args {
    struct initialize {
      int client_key;
      int shmid;
    } initialize;
    struct finish {
      int client_key;
    } finish;
  } msgdata;
} tiny_msgbuf;

LIST_HEAD(tiny_client_list, tiny_client);

typedef struct tiny_client {
  int client_number;
  int client_key;
  int client_msgqid;
  int shmid;
  void *shm;
  LIST_ENTRY(tiny_client) next_client;
} tiny_client;

typedef struct shm_header {
  int magic_value;
  int used;

  size_t uncompressed_length;
  size_t compressed_length;
} shm_header;

#endif
