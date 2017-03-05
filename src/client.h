#ifndef TINYFILE_CLIENT
#define TINYFILE_CLIENT

#include "tiny.h"

int tiny_initialize();
void tiny_compress();
void tiny_uncompress();
void tiny_compress_async();
void tiny_uncompress_async();
void tiny_finish();

typedef struct tiny_async_args {
  int task; // 0=compress, 1=uncompress
  char *inbuf;
  size_t insz;

  char *outbuf;
  size_t outsz;
} tiny_async_args;

typedef struct tiny_notifier {
  void (*notify_function)(tiny_async_args, void*);
  void *notify_args;
} tiny_notifier;

#endif
