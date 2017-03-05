#include "client.h"

void handle_done(void *args) {
  (*((int*) args))++;
  printf("Done compressing! %d\n", *((int*)args));
}

void handle_done_un(void *args) {
  (*((int*) args))++;
  printf("Done uncompressing! %d\n", *((int*)args));
}

int main(int argc, char *argv[]) {
  tiny_notifier notif;
  int event_count = 0; 

  notif.notify_function = handle_done;
  notif.notify_args = &event_count;

  tiny_initialize();
  tiny_compress();
  tiny_uncompress();
  tiny_compress();
  tiny_uncompress();
  tiny_compress();
  tiny_uncompress();

  tiny_compress_async(notif);

  notif.notify_function = handle_done_un;
  usleep(100000);
  tiny_uncompress_async(notif);
  usleep(200000);
  tiny_finish();
}
