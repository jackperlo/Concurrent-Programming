#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));CLEAN;exit(-1);}

/* variabili di dimensionamento della matrice mappa */ 
int SO_HEIGHT, SO_WIDTH;

/* puntatore a matrice che determina la mappa in esecuzione */
int **map; 


/* struct to map the Map passed from shdmem into a local array */
typedef struct{
    int value;
}mapping; 

#define MSG_LEN 128
/* struttura del buffer della coda di messaggi */
struct msgbuf msg_buffer;

struct sembuf s_queue_buff[1];

union semun {
    int val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO*/

};

union semun sem_arg;

int sem_queue_id = 0;
int sem_cells_id = 0;