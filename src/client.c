#include "client.h"

int key;
int msgqid;
int semid;
key_t daemonkey;
int daemonq;
tiny_msgbuf msg;
void *shm;

int tiny_initialize() {
  if ((daemonkey = ftok(MSGQFILE, 'b')) == -1) {
    perror("[CLIENT] ftok error, please check that the tinyfile daemon is running.");
    return -1;
  }

  if ((daemonq = msgget(daemonkey, 0666)) == -1) {
    perror("[CLIENT] msgget");
    return -1;
  }

  msg.mtype = MSG_INIT_REQUEST_TYPE;
  msg.msgdata.initialize.client_key = 0;
  if ((msgsnd(daemonq, &msg, sizeof(tiny_msgbuf), 0)) == -1) {
    perror("[CLIENT] msgsnd");
    return -1;
  }

  if ((msgrcv(daemonq, &msg, sizeof(tiny_msgbuf), MSG_INIT_RESPONSE_TYPE, 0)) == -1) {
    perror("[CLIENT] msgrcv");
    return -1;
  }

  key = msg.msgdata.initialize.client_key;
  int shmid = msg.msgdata.initialize.shmid;

  if ((msgqid = msgget(key, 0666 & !IPC_CREAT)) == -1) {
    perror("[CLIENT] msgget");
    return -1;
  }

  if ((semid = semget(key, 2, 0666 & !IPC_CREAT)) == -1) {
    perror("[CLIENT] semget");
    return -1;
  }

  shm = shmat(shmid, (void *)0, 0);
  if (shm == (char *)(-1)) {
    perror("[CLIENT] shmat");
    return -1;
  }
  return 0;
}

void tiny_compress(char *inbuf, size_t insz, char *outbuf, size_t *outsz) {
  tiny_msgbuf msg;
  char *input_buf;
  struct sembuf semargs;

  start();
  semargs.sem_num = 0; // Try to acquire the compression semaphore.
  semargs.sem_op = -1; // Wait until the semaphore has at least one available, then make it zero
  semop(semid, &semargs, 1);
  /* while (((shm_header*) shm)->used > 0) {} */
  {
    input_buf = ((char*) shm) + sizeof(shm_header);
    memcpy(input_buf, inbuf, insz);
        
    ((shm_header*) shm)->uncompressed_length = insz;
    ((shm_header*) shm)->compressed_length = -1;

    msg.mtype = MSG_CMP_TYPE;
    msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);
  }
    
  semargs.sem_num = 1; // Wait for the compression done semaphore.
  semargs.sem_op = -1; // Wait until the semaphore has at least one available, then make it zero
  semop(semid, &semargs, 1);
  end();

  *outsz = ((shm_header*) shm)->compressed_length;
  fprintf(stderr, "{\"op\": \"compress_blocking\", \"total_time\": %f, \"ipc_time\": %f, \"file_size\": %zu},\n", TIME, TIME - ((shm_header*) shm)->snappy_time, insz);

  memcpy(outbuf, input_buf, *outsz);

  // Release only after safely copied out.
  semargs.sem_num = 0; // Compression semaphore.
  semargs.sem_op = 1; // Release.
  semop(semid, &semargs, 1);
}

void tiny_uncompress(char *inbuf, size_t insz, char *outbuf, size_t *outsz) {
  tiny_msgbuf msg;
  char *input_buf;
  struct sembuf semargs;

  start();
  semargs.sem_num = 0; // Try to acquire the compression semaphore.
  semargs.sem_op = -1; // Wait until the semaphore has at least one available, then make it zero
  semop(semid, &semargs, 1);
  {
    input_buf = ((char*) shm) + sizeof(shm_header);
    memcpy(input_buf, inbuf, insz);
        
    ((shm_header*) shm)->compressed_length = insz;
    ((shm_header*) shm)->uncompressed_length = -1;

    msg.mtype = MSG_UNCMP_TYPE;
    msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);
  }

  semargs.sem_num = 1; // Wait for the uncompression done semaphore.
  semargs.sem_op = -1; // Wait until the semaphore has at least one available, then make it zero
  semop(semid, &semargs, 1);
  end();

  *outsz = ((shm_header*) shm)->uncompressed_length;
  fprintf(stderr, "{\"op\": \"uncompress_blocking\", \"total_time\": %f, \"ipc_time\": %f, \"file_size\": %zu},\n", TIME, TIME - ((shm_header*) shm)->snappy_time, *outsz);

  memcpy(outbuf, input_buf, *outsz);

  // Release only after safely copied out.
  semargs.sem_num = 0; // Compression semaphore.
  semargs.sem_op = 1; // Release.
  semop(semid, &semargs, 1);
}

void tiny_compress_async(char *inbuf, size_t insz, tiny_notifier notif) {
  if (!fork()) {
    tiny_async_args ctx;
    ctx.task = 0;
    ctx.inbuf = inbuf;
    ctx.insz = insz;
    ctx.outsz = -1;
    ctx.outbuf = (char*) malloc(ctx.insz * 2);
    tiny_compress(ctx.inbuf, ctx.insz, ctx.outbuf, &ctx.outsz);
    notif.notify_function(ctx, notif.notify_args);
    free(ctx.outbuf);
    exit(1);
  } else {
    return;
  }
}

void tiny_uncompress_async(char *inbuf, size_t insz, tiny_notifier notif) {
  if (!fork()) {
    tiny_async_args ctx;
    ctx.task = 1;
    ctx.inbuf = inbuf;
    ctx.insz = insz;
    ctx.outsz = -1;
    ctx.outbuf = (char*) malloc(ctx.insz * 2);
    tiny_uncompress(ctx.inbuf, ctx.insz, ctx.outbuf, &ctx.outsz);
    notif.notify_function(ctx, notif.notify_args);
    free(ctx.outbuf);
    exit(1);
  } else {
    return;
  }
}

void tiny_finish() {
  if (shmdt(shm) < 0) {
    perror("[CLIENT] shmdt");
    exit(1);
  }

  tiny_msgbuf msg;
  msg.mtype = MSG_FIN_TYPE;
  msg.msgdata.finish.client_key = key;
  msgsnd(daemonq, &msg, sizeof(tiny_msgbuf), 0);
}
