#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include<pthread.h>
#include <stdlib.h>

#define URL         "duydk"
#define SEMKEYPATH "/dev/null"
#define SEMKEYID 1
#define NUMSEMS 2
const int SIZE = 100;

void * ptrShm;
int semFd;
char conten[150];
char msg[200];

typedef struct {
    int size;
    char *name;
} stName_t;

void exit_handle() {
    // unmap share memory
    if (ptrShm != NULL) {
        if (munmap(ptrShm, SIZE) == -1) {
            printf("munmap failed\n");
        }
    }
    exit(1);
}

void signal_handler(int signum) {
    exit_handle();
}


int main (int argc, char *argv[]) {
    struct sembuf operations[NUMSEMS];

    if (argc < 2) {
        printf("non-option ARGV-elements: ");
        return -1;
    }

    // handle signal terminate
    signal(SIGINT, signal_handler);

    // get user name
    stName_t user;
    user.size = strlen(argv[1]);
    user.name = malloc(user.size);
    strcpy(user.name, argv[1]);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("wellcome %s\n", user.name);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    // Create a share memory
    // Create file fd
    int fd = shm_open(URL, O_RDWR, 0666);
    if (fd == -1) {
        printf("Cannot open URL\n");
        return -1;
    }
    // map shared memory to process
    ptrShm = mmap(NULL, SIZE, PROT_WRITE, MAP_SHARED, fd, 0);

    // create semaphore
    key_t semkey = ftok(SEMKEYPATH, SEMKEYID);
    int semFd = semget( semkey, NUMSEMS, 0666);
    if (semFd == -1) {
        printf("Cannot get semaphore\n");
        return -1;
    }

    printf("Start typing .....\n");
    while (1) {
        printf("\n %s: ", user.name);
        fflush(stdin);
        gets(conten);
        operations[0].sem_num = 1;
        operations[0].sem_op = 0;
        operations[0].sem_flg = 0;
        operations[1].sem_num = 1;
        operations[1].sem_op = 1;
        operations[1].sem_flg = IPC_NOWAIT;
        if (semop( semFd, operations, 2) == -1) {
            printf("semop() failed\n");
            exit_handle();
        }
        sprintf(ptrShm,"%s :%s", user.name, conten);
        operations[0].sem_num = 0;
        operations[0].sem_op = -1;
        operations[0].sem_flg = IPC_NOWAIT;
        if (semop( semFd, operations, 1) == -1) {
            printf("semop() failed\n");
            exit_handle();
        }
    }
}