#include "client.h"

int key;
int msgqid;
key_t daemonkey;
int daemonq;

void tiny_initialize() {
  if((daemonkey = ftok(MSGQFILE, 'b')) == -1) {
    perror("[CLIENT] ftok error, please check that the tinyfile daemon is running.");
    exit(1);
  }

  if((daemonq = msgget(daemonkey, 0666)) == -1) {
    perror("[CLIENT] msgget");
    exit(1);
  }

  tiny_msgbuf msg;
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

  if((msgqid = msgget(key, 0666 & !IPC_CREAT)) == -1) {
    perror("[CLIENT] msgget");
    exit(1);
  }
}

void tiny_finish() {
  tiny_msgbuf msg;
  msg.mtype = MSG_FIN_TYPE;
  msg.msgdata.finish.client_key = key;
  msgsnd(daemonq, &msg, sizeof(tiny_msgbuf), 0);
}

void tiny_compress() {
  // TODO: Set up shared memory to copy input into, semaphores to control returning etc.
  tiny_msgbuf msg;
  msg.mtype = MSG_CMP_TYPE;
  msg.msgdata.compress_args.input = 0;
  msg.msgdata.compress_args.input_length = 0;
  msg.msgdata.compress_args.compressed = 0;
  msg.msgdata.compress_args.compressed_length = 0;
  msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);
}

void tiny_uncompress() {
  // TODO: Set up shared memory to copy input into, semaphores to control returning etc.
  tiny_msgbuf msg;
  msg.mtype = MSG_UNCMP_TYPE;
  msg.msgdata.uncompress_args.compressed = 0;
  msg.msgdata.uncompress_args.length = 0;
  msg.msgdata.uncompress_args.uncompressed = 0;
  msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);
}
