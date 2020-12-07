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

#ifndef SO_HEIGHT
#define SO_HEIGHT 10
#endif

#ifndef SO_WIDTH
#define SO_WIDTH 10
#endif

int **map; /* puntatore a matrice che determina la mappa in esecuzione */

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));CLEAN;exit(-1);}

/* struct per mappare la Map passata dalla shdmem in un array locale */
typedef struct{
    int value;
}mapping; 

#define MSG_LEN 128
/* struttura del buffer della coda di messaggi */
struct msgbuf msg_buffer;

struct sembuf s_queue_buff[1]; /* [0] mutex per coda di messaggi + [1] contatore(=SO_SOURCES) per sincronizzazione sources */
struct sembuf s_cells_buff[1]; /* semafori per ogni cella: cosi da sapere quanti taxi ci stanno "fisicamente" in ogni cella */

union semun {
    int val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO*/
};

union semun sem_arg;

/* id dei semafori */
int sem_sync_id = 0; 
int sem_cells_id = 0; 