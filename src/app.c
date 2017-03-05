#include "client.h"
#include <sys/sem.h>

void handle_done_un(tiny_async_args ctx, void *args) {
  struct sembuf semargs;
  int semid = *((int*)args);
  printf("Done uncompressing! %zu bytes -> %zu bytes\n", ctx.insz, ctx.outsz);
  /* free(ctx.inbuf); */
  semargs.sem_num = 0;
  semargs.sem_op = -1;
  semop(semid, &semargs, 1);
  printf("%d processes left\n", semctl(semid, 0, GETVAL, NULL));
}

void handle_done(tiny_async_args ctx, void *args) {
  printf("Done compressing! %zu bytes -> %zu bytes\n", ctx.insz, ctx.outsz);

  // dispatch uncompress IPC
  tiny_notifier notif;
  notif.notify_args = args;
  notif.notify_function = handle_done_un;

  char *buf = (char *) malloc(ctx.outsz * sizeof(char));
  memcpy(buf, ctx.outbuf, ctx.outsz);
  tiny_uncompress_async(buf, ctx.outsz, notif);
}

int main(int argc, char *argv[]) {
  struct sembuf semargs;
  int semid;
  int asyncFlag = 0;
  int nfiles = 0;
  char *filenames[512];
    
  int index;
  int c;

  opterr = 0;

  if(argc < 2) {
    printf("Please supply filenames to process\n");
  }

  semid = semget(ftok("./bin/tiny", 'b'), 1, 0666 | IPC_CREAT);
  printf("App has semaphore set with id %d\n", semid);
  semargs.sem_num = 0;

  while ((c = getopt(argc, argv, "an:s:")) != -1)
    switch (c) {
    case 'a':
      asyncFlag = 1;
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
      semctl(semid, 0, IPC_RMID, NULL);
      abort();
    }

  nfiles = argc - optind;
  assert(nfiles > 0 && nfiles < 513);
  printf ("async = %d len(files) = %d\n", asyncFlag, nfiles);

  for (index = optind; index < argc; index++)
    filenames[index - optind] = argv[index];

  /*****************************************
   * BEGIN APPLICATION
   *****************************************/

  FILE *fileptr;
  char *filebuf;
  size_t filelen;

  char *outbuf;
  size_t outlen;

  tiny_notifier notif;

  if(tiny_initialize() < 0) {
    semctl(semid, 0, IPC_RMID, NULL);
    exit(1);
  }

  semargs.sem_op = nfiles;
  semop(semid, &semargs, 1);
  for (index = 0; index < nfiles; ++index) {
    if ((fileptr = fopen(filenames[index], "rb")) == NULL) {
      perror("fopen (couldn't find file?): ");
      semctl(semid, 0, IPC_RMID, NULL);
      tiny_finish();
      exit(1);
    }
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    filebuf = (char*) malloc((filelen) * sizeof(char));
    fread(filebuf, filelen, 1, fileptr);
    fclose(fileptr);

    if (!asyncFlag) { // blocking mode
      outbuf = (char*) malloc((filelen) * sizeof(char) * 2); // compressed could be bigger than input size!
            
      tiny_compress(filebuf, filelen, outbuf, &outlen);
      tiny_uncompress(outbuf, outlen, outbuf, &outlen);

      printf("%s (", filenames[index]);
      if (outlen < 1024) {
        for(int i = 0; i < filelen; i++)
          printf("%02x", filebuf[i]);
        printf("; ");
        for(int i = 0; i < outlen; i++)
          printf("%02x", outbuf[i]);
      } else
        printf("<<OMITTED>>");
      printf(") => %s\n", memcmp(filebuf, outbuf, filelen) ? "mismatch" : "match");
            
      free(outbuf);
    } else { // async mode: dispatch compression event requests
      notif.notify_function = handle_done;
      notif.notify_args = &semid;
      tiny_compress_async(filebuf, filelen, notif);
    }
    free(filebuf);
  }

  if(asyncFlag) {
    semargs.sem_op = 0;
    semop(semid, &semargs, 1);
  }

  semctl(semid, 0, IPC_RMID, NULL);
  tiny_finish();
  exit(0);
}
