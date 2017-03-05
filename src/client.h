#ifndef TINYFILE_CLIENT
#define TINYFILE_CLIENT

#include "tiny.h"

void tiny_initialize();
void tiny_compress();
void tiny_uncompress();
void tiny_compress_async();
void tiny_uncompress_async();
void tiny_finish();

typedef struct tiny_notifier {
  void (*notify_function)(void*);
  void *notify_args;
} tiny_notifier;

#endif
