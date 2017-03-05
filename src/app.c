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
    int asyncFlag = 0;
    int nfiles = 0;
    char *filenames[32];
    
    int index;
    int c;

    opterr = 0;

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
            abort();
        }

    nfiles = argc - optind;
    assert(nfiles > 0 && nfiles < 33);
    printf ("async = %d len(files) = %d\n", asyncFlag, nfiles);

    for (index = optind; index < argc; index++)
        filenames[index - optind] = argv[index];

    /*****************************************
    * BEGIN APPLICATION
    *****************************************/

    FILE *fileptr;
    char *buffer;
    long filelen;
    
    // tiny_notifier notif;
    // int event_count = 0; 
    // notif.notify_function = handle_done;
    // notif.notify_args = &event_count;

    tiny_initialize();

    for (index = 0; index < nfiles; ++index) {
        fileptr = fopen(filenames[index], "rb");
        fseek(fileptr, 0, SEEK_END);
        filelen = ftell(fileptr);
        rewind(fileptr);

        buffer = (char* )malloc((filelen+1) * sizeof(char));
        fread(buffer, filelen, 1, fileptr);
        fclose(fileptr);
        
        if (!asyncFlag) { // blocking mode
            
        } else { // async mode

        }

        free(buffer);
    }

    // tiny_compress();
    // tiny_uncompress();
    // tiny_compress();
    // tiny_uncompress();
    // tiny_compress();
    // tiny_uncompress();

    // tiny_compress_async(notif);

    // notif.notify_function = handle_done_un;
    // usleep(100000);
    // tiny_uncompress_async(notif);
    // usleep(200000);
    // tiny_finish();
}
