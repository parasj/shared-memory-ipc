#include "snappy.h"
#include "tiny.h"
typedef struct snappy_env snappy_env;

int msqid = 0;
int last_client = 0;

struct tiny_client_list clients;
void sigint_handler(int sig) {
  remove(MSGQFILE);
  if(msgctl(msqid, IPC_RMID, NULL) == -1) {
    perror("[SERVER] msgctl");
  } else {
    fprintf(stderr, "[SERVER] Successfully closed message queue\n");
  }

  tiny_client *i;
  for(i = clients.lh_first; i != NULL;
      i = i->next_client.le_next) {
    char client_file[50];
    sprintf(client_file, CLIENT_MSGQFILE_FMT, i->client_number);
    remove(client_file);
    if(msgctl(i->client_msgqid, IPC_RMID, NULL) == -1) {
      perror("[SERVER] msgctl");
    } else {
      fprintf(stderr, "[SERVER] Successfully closed message queue for client #%d\n",
              i->client_number);
    }
  }

  exit(1);
}

void initialize() {
  key_t key;
  FILE *fp = fopen(MSGQFILE, "ab+");
  if(!fp) {
    perror("[SERVER] fopen");
  }
   
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

  LIST_INIT(&clients);
}

void new_client_handler() {
  // TODO: Set up shared memory
  tiny_client *client = malloc(sizeof(tiny_client));
  client->client_number = last_client;
  char client_file[50];
  sprintf(client_file, CLIENT_MSGQFILE_FMT, last_client);

  FILE *fp = fopen(client_file, "ab+");
  if(!fp) {
    perror("[SERVER] new client fopen");
    exit(1);
  }
   
  if((client->client_key = ftok(client_file, 'b')) == -1) {
    perror("[SERVER] new client ftok");
    exit(1);
  }

  if((client->client_msgqid = msgget(client->client_key,
                                     0666 | IPC_CREAT)) == -1) {
    perror("[SERVER] new client msgget");
    exit(1);
  }

  LIST_INSERT_HEAD(&clients, client, next_client);
  last_client++;

  tiny_msgbuf msg;
  msg.mtype = MSG_INIT_RESPONSE_TYPE;
  msg.msgdata.initialize.client_key = client->client_key;
  fprintf(stdout, "[SERVER] Successfully added new client number %d with key %d\n", client->client_number, client->client_key);
  msgsnd(msqid, &msg, sizeof(tiny_msgbuf), 0);
}

void remove_client_handler(tiny_msgbuf *msg) {
  tiny_client *i;
  for(i = clients.lh_first; i != NULL;
      i = i->next_client.le_next) {
    if(i->client_key == msg->msgdata.finish.client_key) {
      char client_file[50];
      sprintf(client_file, CLIENT_MSGQFILE_FMT, i->client_number);
      remove(client_file);
      if(msgctl(i->client_msgqid, IPC_RMID, NULL) == -1) {
        perror("[SERVER] msgctl");
      } else {
        fprintf(stderr, "[SERVER] Successfully closed message queue for client #%d\n",
                i->client_number);
      }
      return;
    }
  }

  fprintf(stderr, "[SERVER] Asked to remove non-existent client number %d\n", msg->msgdata.finish.client_key);
}

void compress_handler(char *input, size_t input_length,
                      char *compressed,
                      size_t *compressed_length) {
  // TODO: Actually do the compression
  fprintf(stderr, "{type: 'compress', input: %p, input_length: %lu,"
          "compressed: %p, compressd_length: %p}\n",
          input, input_length, compressed, compressed_length);
}

void uncompress_handler(char *compressed, size_t length,
                        char *uncompressed) {
  // TODO: Actually do the decompression
  fprintf(stderr, "{type: 'uncompress', compressed: %p,"
          "length: %lu, uncompressed: %p}\n",
          compressed, length, uncompressed);
}

void serve() {
  tiny_msgbuf r;
  while(1) {
    // TODO: Serve all of the client queues you have 
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
    case MSG_INIT_REQUEST_TYPE: {
      new_client_handler();
      break;
    }
    case MSG_FIN_TYPE: {
      remove_client_handler(&r);
      break;
    }
    default: {
      fprintf(stderr, "Don't recognize message type %ld \n",
              r.mtype);
      break;
    }
    }
  }
}
 
int main(int argc, char *argv[]) {
  initialize();
  serve();
}
