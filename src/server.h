#ifndef TINYFILE_SERVER
#define TINYFILE_SERVER

#define MAX_FILENAME_SIZE 100;

#include <sys/msg.h>
#include "snappy.h"

const char* msgqfile = ".tiny_msgqfile";

void initialize();
void compress_handler();
void uncompress_handler();

typedef struct snappy_env snappy_env;
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
