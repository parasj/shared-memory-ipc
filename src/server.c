#include "snappy.h"
#include "tiny.h"
typedef struct snappy_env snappy_env;

int msqid = 0;

void sigint_handler(int sig) {
  if(msgctl(msqid, IPC_RMID, NULL) == -1) {
    perror("[SERVER] msgctl");
  } else {
    fprintf(0, "[SERVER] Successfully closed message queue\n");
  }
  exit(1);
}

void initialize() {
  key_t key;
  if((key = ftok(MSGQFILE, 'b')) == -1) {
    perror("[SERVER] ftok");
    exit(1);
  }

  if((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
    perror("[SERVER] msgget");
    exit(1);
  }

  void sigint_handler(int sig);
  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);

  if(sigaction(SIGINT, &sa, NULL) == -1) {
    perror("[SERVER] sigaction");
    exit(1);
  }

  if(sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("[SERVER] sigaction");
    exit(1);
  }
}

void compress_handler(char *input, size_t input_length,
                      char *compressed,
                      size_t *compressed_length) {
  fprintf(stderr, "{type: 'compress', input: %p, input_length: %lu,"
          "compressed: %p, compressd_length: %p}\n",
          input, input_length, compressed, compressed_length);
}

void uncompress_handler(char *compressed, size_t length,
                        char *uncompressed) {
  fprintf(stderr, "{type: 'uncompress', compressed: %p,"
          "length: %lu, uncompressed: %p}\n",
          compressed, length, uncompressed);
}

void serve() {
  tiny_msgbuf r;
  while(1) {
    msgrcv(msqid, &r, sizeof(tiny_msgbuf), 0, 0);
    switch(r.mtype) {
    case MSG_CMP_TYPE: {
      compress_handler(r.msgdata.compress_args.input,
                       r.msgdata.compress_args.input_length,
                       r.msgdata.compress_args.compressed,
                       r.msgdata.compress_args.compressed_length);
      break;
    }
    case MSG_UNCMP_TYPE: {
      uncompress_handler(r.msgdata.uncompress_args.compressed,
                         r.msgdata.uncompress_args.length,
                         r.msgdata.uncompress_args.uncompressed);
      break;
    }
    default: {
      fprintf(stderr, "Don't recognize message type %ld \n", r.mtype);
      break;
    }
    }
  }
}
 
int main(int argc, char *argv[]) {
  initialize();
  serve();
}
