#include "client.h"

int key;
int msgqid;
key_t daemonkey;
int daemonq;
tiny_msgbuf msg;

void *shm;

void tiny_initialize() {
    if ((daemonkey = ftok(MSGQFILE, 'b')) == -1) {
        perror("[CLIENT] ftok error, please check that the tinyfile daemon is running.");
        exit(1);
    }

    if ((daemonq = msgget(daemonkey, 0666)) == -1) {
        perror("[CLIENT] msgget");
        exit(1);
    }

    msg.mtype = MSG_INIT_REQUEST_TYPE;
    msg.msgdata.initialize.client_key = 0;
    if ((msgsnd(daemonq, &msg, sizeof(tiny_msgbuf), 0)) == -1) {
        perror("[CLIENT] msgsnd");
        exit(1);
    }

    if ((msgrcv(daemonq, &msg, sizeof(tiny_msgbuf), MSG_INIT_RESPONSE_TYPE, 0)) == -1) {
        perror("[CLIENT] msgrcv");
        exit(1);
    }

    key = msg.msgdata.initialize.client_key;
    int shmid = msg.msgdata.initialize.shmid;

    if ((msgqid = msgget(key, 0666 & !IPC_CREAT)) == -1) {
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

void tiny_compress(char *inbuf, size_t insz, char *outbuf, size_t *outsz) {
    tiny_msgbuf msg;
    char *input_buf;

    while (((shm_header*) shm)->used > 0)
        usleep(10000);

    {
        ((shm_header*) shm)->used = 1;
        input_buf = ((char*) shm) + sizeof(shm_header);
        memcpy(input_buf, inbuf, insz);
        
        ((shm_header*) shm)->uncompressed_length = insz;
        ((shm_header*) shm)->compressed_length = -1;

        msg.mtype = MSG_CMP_TYPE;
        msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);
    }
    
    while (((shm_header*) shm)->used < 2)
        usleep(10000);

    *outsz = ((shm_header*) shm)->compressed_length;
    memcpy(outbuf, input_buf, *outsz);

    ((shm_header*) shm)->used = 0;
}

void tiny_uncompress(char *inbuf, size_t insz, char *outbuf, size_t *outsz) {
    tiny_msgbuf msg;
    char *input_buf;

    while (((shm_header*) shm)->used > 0)
        usleep(10000);

    {
        ((shm_header*) shm)->used = 1;
        
        input_buf = ((char*) shm) + sizeof(shm_header);
        memcpy(input_buf, inbuf, insz);
        
        ((shm_header*) shm)->compressed_length = insz;
        ((shm_header*) shm)->uncompressed_length = -1;

        msg.mtype = MSG_UNCMP_TYPE;
        msgsnd(msgqid, &msg, sizeof(tiny_msgbuf), 0);
    }

    while (((shm_header*) shm)->used < 2)
        usleep(10000);

    *outsz = ((shm_header*) shm)->uncompressed_length;
    memcpy(outbuf, input_buf, *outsz);

    ((shm_header*) shm)->used = 0;
}

void tiny_compress_async(tiny_notifier notif) {
    if (!fork()) {
        // tiny_compress(); // todo
        notif.notify_function(notif.notify_args);
        exit(1);
        return;
    } else {
        return;
    }
}

void tiny_uncompress_async(tiny_notifier notif) {
    if (!fork()) {
        // tiny_uncompress(); // todo
        notif.notify_function(notif.notify_args);
        exit(1);
        return;
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
