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

#define VALUES_TO_SOURCE 0
#define VALUES_TO_TAXI 1
#define TAXI_RETURNED_VALUES 2

int **map; /* puntatore a matrice che determina la mappa in esecuzione */
int **SO_TIMENSEC_MAP; /* puntatore a matrice dei tempi di attesa per ogni cella */
int **SO_TOP_CELLS_MAP; /* matrice che contiene per ogni cella il numero di volte in cui la cella è stata attraversata */

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));CLEAN;exit(-1);}

/* struct per mappare la Map passata dalla shdmem in un array locale */
typedef struct{
    int cell_map_value; /* mapping della matrice map: contiene i valori di ogni cella della matrice */
}values_to_source;

/* struct per passare con una shared mem due matrici (che sono essenziali per l'esecuzione dei taxi) dal master ai taxi*/
typedef struct{
	int cell_map_value; /* mapping della matrice map: contiene i valori di ogni cella della matrice */ 
	int cell_timensec_map_value; /* mapping della matrice SO_TIMENSEC_MAP che contiene il tempo impiegato da un taxi per attraversare la cella corrispondente */
}values_to_taxi;

typedef union{
	int completed_trips_counter; /* numero di viaggi completati dal taxi */
	int max_timensec_complete_trip_value; /* tempo massimo impiegato per completare un viaggio */
	int max_cells_crossed_longest_trip_value; /* numero massimo di celle attraversate in un viaggio */
	int cell_crossed_map_counter; /* mapping della matrice che contiene il numero di volte in cui ogni cella è stata attrversata da un taxi */
} taxi_returned_values;

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