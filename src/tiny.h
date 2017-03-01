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

#define MSGQFILE "/tmp/.tiny_msgqfile"
#define MSG_CMP_TYPE 1
#define MSG_UNCMP_TYPE 2

typedef struct tiny_msgbuf {
  long mtype;
  union tiny_args {
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

#endif
