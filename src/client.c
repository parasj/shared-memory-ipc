#include "client.h"

int key;
int msgqid;
key_t daemonkey;
int daemonq;
tiny_msgbuf msg;

int shm_slots = 1;
size_t shm_size = 1024 * 1024 * 2; // 4mb slots

void *shm;

void tiny_initialize() {
  if((daemonkey = ftok(MSGQFILE, 'b')) == -1) {
    perror("[CLIENT] ftok error, please check that the tinyfile daemon is running.");
    exit(1);
  }

  if((daemonq = msgget(daemonkey, 0666)) == -1) {
    perror("[CLIENT] msgget");
    exit(1);
  }

  msg.mtype = MSG_INIT_REQUEST_TYPE;
  msg.msgdata.initialize.client_key = 0;
  if((msgsnd(daemonq, &msg, sizeof(tiny_msgbuf), 0)) == -1) {
    perror("[CLIENT] msgsnd");
    exit(1);
  }

  if((msgrcv(daemonq, &msg, sizeof(tiny_msgbuf), MSG_INIT_RESPONSE_TYPE, 0)) == -1) {
    perror("[CLIENT] msgrcv");
    exit(1);
  }

  key = msg.msgdata.initialize.client_key;
  int shmid = msg.msgdata.initialize.shmid;

  if((msgqid = msgget(key, 0666 & !IPC_CREAT)) == -1) {
    perror("[CLIENT] msgget");
    exit(1);
  }

  shm = shmat(shmid, (void *)0, 0);
  if (shm == (char *)(-1)) {
    perror("[CLIENT] shmat");
    exit(1);
  }

  printf("[CLIENT] magic value %d (should be 123456)\n", ((shm_header*) shm)->magic_value);
  
}

void tiny_finish() {
  tiny_msgbuf msg;
  msg.mtype = MSG_FIN_TYPE;
  msg.msgdata.finish.client_key = key;
  msgsnd(daemonq, &msg, sizeof(tiny_msgbuf), 0);
}

void tiny_compress() {
  tiny_msgbuf msg;

  if (((shm_header*) shm)->used > 0) {
    printf("[CLIENT] shm is used!\n");
  }

  ((shm_header*) shm)->used = 1;
  char *input_buf = ((char*) shm) + sizeof(shm_header);
  strcpy(input_buf, "1234");
  ((shm_header*) shm)->uncompressed_length = sizeof("1234"); // put size here!
  ((shm_header*) shm)->compressed_length = -1;

  msg.mtype = MSG_CMP_TYPE;
  msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);

  while (((shm_header*) shm)->used < 2) {
    usleep(10000);
  }

  printf("GOT COMPRESSED RESULT! size %zu\n", ((shm_header*) shm)->compressed_length);

  ((shm_header*) shm)->used = 0;
}

void tiny_uncompress() {
  tiny_msgbuf msg;

  if (((shm_header*) shm)->used > 0) {
    printf("[CLIENT] shm is used!\n");
  }

  ((shm_header*) shm)->used = 1;
  char *input_buf = ((char*) shm) + sizeof(shm_header);
  strcpy(input_buf, "1234");
  ((shm_header*) shm)->compressed_length = sizeof("1234"); // put size here!
  ((shm_header*) shm)->uncompressed_length = -1;

  msg.mtype = MSG_UNCMP_TYPE;
  msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);

  while (((shm_header*) shm)->used < 2) {
    usleep(10000);
  }

  printf("GOT DECOMPRESSED RESULT! size %zu\n", ((shm_header*) shm)->compressed_length);

  ((shm_header*) shm)->used = 0;
}
