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

struct sembuf s_queue_buff;
union semun sem_arg;

int sem_queue;