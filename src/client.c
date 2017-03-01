#include "client.h"

int msqid = 0;

void tiny_initialize() {
  key_t key;
  if((key = ftok(MSGQFILE, 'b')) == -1) {
    perror("[CLIENT] ftok error, please check that the tinyfile daemon is running.");
    exit(1);
  }

  if((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
    perror("[CLIENT] msgget");
    exit(1);
  }
}

void tiny_compress() {
  tiny_msgbuf msg;
  msg.mtype = MSG_CMP_TYPE;
  msg.msgdata.compress_args.input = 0;
  msg.msgdata.compress_args.input_length = 0;
  msg.msgdata.compress_args.compressed = 0;
  msg.msgdata.compress_args.compressed_length = 0;
  msgsnd(msqid, &msg, sizeof(tiny_msgbuf), 0);
}

void tiny_uncompress() {
  tiny_msgbuf msg;
  msg.mtype = MSG_UNCMP_TYPE;
  msg.msgdata.uncompress_args.compressed = 0;
  msg.msgdata.uncompress_args.length = 0;
  msg.msgdata.uncompress_args.uncompressed = 0;
  msgsnd(msqid, &msg, sizeof(tiny_msgbuf), 0);
}
