#include "server.h"

void initialize() {
  key_t key = ftok(msgqfile, 'b');
  int msqid = msgget(key, 0666 | IPC_CREAT);
}

void compress_handler() {
}

void uncompress_handler() {
  
}
 
int main(int argc, char *argv[]) {
  initialize();
}
