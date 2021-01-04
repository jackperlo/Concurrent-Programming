#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include <time.h>
#include <sys/sem.h>

#define SO_WIDTH 30
#define SO_HEIGHT 30

/* colori di foreground e background */
#define RESET "\x1B[0m"
#define KBLACK "\x1B[30m"
#define KRED "\x1B[31m"
#define KWHT  "\x1B[37m"
#define BGRED "\x1B[101m"
#define BGGREEN "\x1B[102m"
#define BGWHITE "\x1B[107m"
#define BGMAGENTA "\x1B[105m"

#define VALUES_TO_SOURCE 0
#define VALUES_TO_TAXI 1
#define TAXI_RETURNED_VALUES 2
#define TAXI_ABORTED_STATUS 22
#define TAXI_NOT_COMPLETED_STATUS 18
#define MSG_LEN 128
#define GOTO_SOURCE 1
#define CLEAN 

int **map; /* puntatore a matrice che determina la mappa in esecuzione */
						/*VALORI POSSIBILI NELLA MAPPA:*/
						/*
								0->cella inaccessibile
								1->cella vergine
								2->cella source
						*/
long int **SO_TIMENSEC_MAP; /* puntatore a matrice dei tempi di attesa per ogni cella */
int **SO_TOP_CELLS_MAP; /* matrice che contiene per ogni cella il numero di volte in cui la cella è stata attraversata */

/* funzioni e struttura dati per lettura e gestione parametri su file */
typedef struct node {
    char name[16];
	long int value;
	struct node *next;  
} node;
typedef node *param_list; /* conterrà la lista dei parametri passati tramite file di settings */

/* struct per mappare la Map passata dalla shdmem in un array locale */
typedef struct{
    int cell_map_value; /* mapping della matrice map: contiene i valori di ogni cella della matrice */
}values_to_source;

/* struct per passare con una shared mem due matrici (che sono essenziali per l'esecuzione dei taxi) dal master ai taxi*/
typedef struct{
	int cell_map_value; /* mapping della matrice map: contiene i valori di ogni cella della matrice */ 
	long int cell_timensec_map_value; /* mapping della matrice SO_TIMENSEC_MAP che contiene il tempo impiegato da un taxi per attraversare la cella corrispondente */
}values_to_taxi;

/*									0						1									2				             3	    	4			5		6	            	7		...*/				
/* taxi_returned_values = [completed_trips_counter, max_timensec_complete_trip_value, total_n_cells_crossed_value, max_trip_completed, pid [0], pid [1], pid [2], cell_crossed_map_counter[1], ...]; */
typedef union{
	int completed_trips_counter; /* totale numero viaggi completati da tutti i taxi */
	int max_timensec_complete_trip_value; /* tempo massimo impiegato per completare un viaggio */
	int total_n_cells_crossed_value; /* numero totale di celle attraversate durante l'intera vita del taxi */
	int cell_crossed_map_counter; /* mapping della matrice che contiene il numero di volte in cui ogni cella è stata attrversata da un taxi */
	int max_trip_completed; /* processo che ha completato più viaggi: numero di viaggi completati*/
	int pid;
} taxi_returned_values;

struct sembuf s_queue_buff[10]; /* [0] mutex per coda di messaggi | [1] contatore(=SO_SOURCES+SO_TAXI) per sincronizzazione generale figli */

struct sembuf s_cells_buff[10]; /* semafori per ogni cella: cosi da sapere quanti taxi ci stanno "fisicamente" in ogni cella */

struct timespec timeout; /*sfruttata per il contatore alla rovescia "anti-deadlock" nei taxi*/

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));CLEAN;exit(-1);}

#endif