#include "snappy.h"
#include "tiny.h"
typedef struct snappy_env snappy_env;

snappy_env env;

int msqid = 0;
int last_client = 0;

struct tiny_client_list clients;

int shm_slots;
size_t shm_size;

// shared compression/decompression buffer
char *outbuf;

/* catch CTRL-C */

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
      perror("[SERVER] msgctl (already closed?)");
    } else {
      fprintf(stderr, "[SERVER] Successfully closed message queue for client #%d\n",
              i->client_number);
    }
    if (shmdt(i->shm) == -1 || shmctl(i->shmid, IPC_RMID, NULL)) {
      perror("[SERVER] shmdt or shctl (already closed?)");
    } else {
      fprintf(stderr, "[SERVER] Successfully cleaned up shm for client #%d\n", i->client_number);
    }
    free(i);
  }

  snappy_free_env(&env);
  free(outbuf);

  exit(1);
}

void initialize(int nslots, size_t buf_size) {
  key_t key;
  FILE *fp = fopen(MSGQFILE, "ab+");
  if(!fp) {
    perror("[SERVER] fopen");
    exit(1);
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
    perror("[SERVER] sigaction (1)");
    exit(1);
  }

  snappy_init_env(&env);
  LIST_INIT(&clients);

  shm_slots = nslots;
  shm_size = buf_size;

  outbuf = (char*) malloc(shm_size);
}

void new_client_handler() {
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

  if((client->client_msgqid = msgget(client->client_key, 0666 | IPC_CREAT)) == -1) {
    perror("[SERVER] new client msgget");
    exit(1);
  }

  /** SHM init **/
  int shmid;
  size_t shmtot = shm_size * shm_slots + sizeof(shm_header);
  printf("[SERVER] allocating SHM segment size %zu\n", shmtot);
  if ((shmid = shmget(client->client_key, shmtot, 0666 | IPC_CREAT)) == -1) {
    perror("[SERVER] shmget");
    exit(1);
  }

  client->shmid = shmid;
  client->shm = shmat(shmid, (void *)0, 0);
  if (client->shm == (char *)(-1)) {
    perror("[SERVER] shmat");
    exit(1);
  }

  ((shm_header*) client->shm)->magic_value = 123456;
  ((shm_header*) client->shm)->used = 0; // todo

  LIST_INSERT_HEAD(&clients, client, next_client);
  last_client++;

  tiny_msgbuf msg;
  msg.mtype = MSG_INIT_RESPONSE_TYPE;
  msg.msgdata.initialize.client_key = client->client_key;
  msg.msgdata.initialize.shmid = shmid;
  fprintf(stdout, "[SERVER] Successfully added new client number %d with key %d\n",
          client->client_number, client->client_key);
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
      if (shmdt(i->shm) == -1 || shmctl(i->shmid, IPC_RMID, NULL)) {
        perror("[SERVER] shmdt or shctl");
      } else {
        fprintf(stderr, "[SERVER] Successfully cleaned up shm for client #%d\n", i->client_number);
      }
      LIST_REMOVE(i, next_client);
      free(i);
      return;
    }
  }

  fprintf(stderr, "[SERVER] Asked to remove non-existent client number %d\n", msg->msgdata.finish.client_key);
}

double compress_handler(char *input, size_t input_length, char *compressed, size_t *compressed_length) {
  assert(input_length <= shm_size);

  start();

  if (snappy_compress(&env, input, input_length, outbuf, compressed_length) < 0) {
    printf("[SERVER] Snappy compression error");
  } else {
    memcpy(compressed, outbuf, *compressed_length);
  }
  // fprintf(stderr, "{type: 'compress', input: %p, input_length: %lu, compressed: %p, compressed_length: %lu}\n", input, input_length, compressed, *compressed_length);
  
  end();
  fprintf(stderr, "{\"op\": \"snappy_compress\", \"time\": %f, \"file_size\": %zu},\n", TIME, input_length);
  return TIME;
}

double uncompress_handler(char *input, size_t input_length, char *uncompressed, size_t *uncompressed_length) {
  assert(input_length <= shm_size);

  start();

  if (!snappy_uncompressed_length(input, input_length, uncompressed_length)) {
    printf("[SERVER] Snappy decompression length error");
  }

  assert(*uncompressed_length <= shm_size);

  if (snappy_uncompress(input, input_length, outbuf) < 0) {
    printf("[SERVER] Snappy decompression error");
  } else {
    memcpy(uncompressed, outbuf, *uncompressed_length);
  }

  end();
  return TIME;
  // fprintf(stderr, "{type: 'uncompress', compressed: %p, length: %lu, uncompressed: %p, uncompressed_length: %zu}\n", input, input_length, uncompressed, *uncompressed_length);
}

void serve() {
  tiny_msgbuf r;
  tiny_client *c;

  while(1) {
    // Step 1: service pending client registration/deregistration requests on the main message queue
    if (msgrcv(msqid, &r, sizeof(tiny_msgbuf), -MSG_FIN_TYPE, IPC_NOWAIT) > 0) {
      switch(r.mtype) {
        case MSG_INIT_REQUEST_TYPE: {
          new_client_handler();
          break;
        }

        case MSG_FIN_TYPE: {
          remove_client_handler(&r);
          break;
        }

        default: {
          fprintf(stderr, "Don't recognize message type %ld \n", r.mtype);
          break;
        }
      }
    }

    // Step 2: proportionately service each queue RR
    // TODO implement QoS
    LIST_FOREACH(c, &clients, next_client) {
      if (msgrcv(c->client_msgqid, &r, sizeof(tiny_msgbuf), 0, IPC_NOWAIT) > 0) {
        printf("[SERVER] Servicing %d\n", c->client_msgqid);
        switch (r.mtype) {
          case MSG_CMP_TYPE: {
            ((shm_header*) c->shm)->snappy_time =
              compress_handler((char*) c->shm + sizeof(shm_header),
                            ((shm_header*) c->shm)->uncompressed_length,
                            (char*) c->shm + sizeof(shm_header),
                            &(((shm_header*) c->shm)->compressed_length));
            ((shm_header*) c->shm)->used = 2;
            break;
          }
          
          case MSG_UNCMP_TYPE: {
            ((shm_header*) c->shm)->snappy_time =
              uncompress_handler((char*) c->shm + sizeof(shm_header),
                            ((shm_header*) c->shm)->compressed_length,
                            (char*) c->shm + sizeof(shm_header),
                            &(((shm_header*) c->shm)->uncompressed_length));
            ((shm_header*) c->shm)->used = 2;
            break;
          }

          default: {
            fprintf(stderr, "Don't recognize message type %ld (2) \n", r.mtype);
            break;
          }
        }
      }
    }

    // usleep(100); // sleep a bit so we don't hog 100% CPU (because of IPC_NOWAIT)
  }
}

int main(int argc, char *argv[]) {
  int nsegments = 1;
  size_t segsz = 1024 * 1024 * 10;

  int c;

  opterr = 0;

  while ((c = getopt(argc, argv, "n:s:")) != -1)
      switch (c) {
      case 'n':
          nsegments = atoi(optarg);
          break;
      case 's':
          segsz = atoi(optarg);
          break;
      case '?':
          if (optopt == 'c')
              fprintf (stderr, "Option -%c requires an argument.\n", optopt);
          else if (isprint(optopt))
              fprintf (stderr, "Unknown option `-%c'.\n", optopt);
          else
              fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
          return 1;
      default:
          abort();
      }

  printf ("[SERVER] nsegments = %d, segsz = %zu\n", nsegments, segsz);

  initialize(nsegments, segsz);
  serve();
}
