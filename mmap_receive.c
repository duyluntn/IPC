#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define URL         "duydk"
#define SEMKEYPATH "/dev/null"
#define SEMKEYID 1
#define NUMSEMS 2
const int SIZE = 100;

void * ptrShm = NULL;
int semFd;

void exit_handle() {
    printf("\n finish listen...");
    // Immediately remove the semaphore set
    if (semctl(semFd, NUMSEMS, IPC_RMID) == -1){
        printf("\n remove semaphore failed\n");
    }

    // unmap share memory
    if (ptrShm != NULL) {
        if (munmap(ptrShm, SIZE) == -1) {
            printf("munmap failed\n");
        }
    }

    // rm physical memory shared
    if (shm_unlink(URL) == -1) {
        printf("shm_unlink failed\n");
    }
    exit(1);
}
void signal_handler(int signum) {
    exit_handle();
}


int main () {

    struct sembuf operations[NUMSEMS];
    char * str = ">>>>>>>>>>>>>>>>>>>>>>>Display chat<<<<<<<<<<<<<<<<<<<<\n";
    // handle signal terminate
    signal(SIGINT, signal_handler);

    // Create a share memory
    // Create file fd
    int fd = shm_open(URL, O_CREAT|O_RDWR, 0666);
    if (fd == -1) {
        printf("Cannot open URL\n");
        return -1;
    }
    // Set memory size
    if (ftruncate(fd, SIZE) == -1) {
        printf("Truncate failed\n");
        return -1;
    }

    // map shared memory to process
    ptrShm = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    sprintf(ptrShm,"%s ", str);

    // create semaphore
    key_t semkey = ftok(SEMKEYPATH, SEMKEYID);
    int semFd = semget( semkey, NUMSEMS, 0666 | IPC_EXCL );
    if (semFd == -1) {
        int semFd = semget( semkey, NUMSEMS, 0666 | IPC_CREAT | IPC_EXCL );
        if (semFd == -1) {
            printf("Cannot get semaphore\n");
            return -1;
        }
    }

    short  sarray[NUMSEMS] = {0,1};
    if (semctl( semFd, NUMSEMS, SETALL, sarray) == -1) {
        printf("semctl() initialization failed\n");
        return -1;
    }
    while (1) {
        operations[0].sem_num = 0;
        operations[0].sem_op = 0;
        operations[0].sem_flg = 0;
        operations[1].sem_num = 0;
        operations[1].sem_op = 1;
        operations[1].sem_flg = IPC_NOWAIT;
        if (semop( semFd, operations, 2) == -1) {
            printf(" Waiting semop() failed\n");
            exit_handle();
        }
        printf("\n %s \n", (char *) ptrShm);
        usleep(1000);
        operations[0].sem_num = 1;
        operations[0].sem_op = -1;
        operations[0].sem_flg = IPC_NOWAIT;
        if (semop( semFd, operations, 1) == -1) {
            printf("semop() failed\n");
            exit_handle();
        }
    }

    return 0;
}