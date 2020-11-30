#include <stdio.h>

#include "Source.h"
#include "Taxi.h"

/*-------------CONTROLLO DEFINE--------------*/
#ifndef SO_HEIGHT
#define SO_HEIGHT 10 /* indica l'altezza della matrice che rappresenta la mappa, se non è definito imposto un valore arbitrario di 10 */
#endif

#ifndef SO_WIDTH
#define SO_WIDTH 10 /* indica la larghezza della matrice che rappresenta la mappa, se non è definito imposto un valore arbitrario di 10 */
#endif


void map_generator(int map[SO_HEIGHT][SO_WIDTH], int SO_HOLES, int SO_SOURCES); 
/*
    genera la matrice con annesse celle HOLES e celle SO_SOURCES
*/

void print_map(int map[SO_HEIGHT][SO_WIDTH], int isTerminal);
/*
    stampa una vista della mappa durante l'esecuzione, e con isTerminal evidenzia le SO_TOP_CELLS celle con più frequenza di passaggio
*/

/*-------------COSTANTI GLOBALI-------------*/
const int SO_HOLES; /* indica il numero di celle inaccessibili(da cui holes) all'interno della matrice */ 
const int SO_SOURCES; /* indica il numero di celle sorgenti di possibili richieste. Come fossero le stazioni di servizio per i taxi */
const int SO_TAXI; /* numero di taxi presenti nella sessione in esecuzione */
const int SO_CAP_MIN; /* capacità minima assumibile da SO_CAP: determina il minimo numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
const int SO_CAP_MAX; /* capacità massima assumibile da SO_CAP: determina il MASSIMO numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
const int SO_TIMESEC_MIN; /* valore minimo assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */
const int SO_TIMESEC_MAX; /* valore MASSIMO assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */

/*-------------VARIABILI GLOBALI-------------*/
int SO_TIMEOUT; /* numero di secondi dopo i quali il processo viene abortito */
int SO_DURATION; /* durata dell'esecuzione in secondi */
int SO_TOP_CELLS; /* numero di celle più attraversate (es. 4 -> riferisco le prime 4 più attraversate) */
int SO_TOP_ROAD; /* processo che ha fatto più strada */
int SO_TOP_LENGTH; /* processo che ha impiegato più tempo in un viaggio */
int SO_TOP_REQ; /* processo che ha completato più request di clienti */ 
int SO_TRIP_SUCCESS; /* numero di viaggi eseguiti con successo, da stampare a fine dell'esecuzione */
int SO_TRIP_NOT_COMPLETED; /* numero di viaggi ancora da eseguire o in itinere nel momento della fine dell'esecuzione */
int SO_TRIP_ABORTED; /* numero di viaggi abortiti a causa del deadlock */

int main(int argc, char *argv[]){

    /* TEST DI PRINT_MAP */
    int map[SO_HEIGHT][SO_WIDTH] = {{1,2,3,0},{0,1,1,1},{2,1,1,0},{1,2,1,0}}; /* Mappa di TEST da RIMUOVERE */
    print_map(map, 0);
    
    return 0;
}

void map_generator(int map[SO_HEIGHT][SO_WIDTH], int SO_HOLES, int SO_SOURCES){

}

void print_map(int map[SO_HEIGHT][SO_WIDTH], int isTerminal){
    /* indici per ciclare */
    int i, k;

    /* cicla per tutti gli elementi della mappa */
    for(i = 0; i < SO_HEIGHT; i++){
        for(k = 0; k < SO_WIDTH; k++){
            switch (map[i][k])
            {
            /* CASO 0: cella invalida, quadratino nero */
            case 0:
                printf("□");
                break;
            /* CASO 1: cella di passaggio valida, non sorgente, quadratino bianco */
            case 1:
                printf("■");
                break;
            /* CASO 2: cella sorgente, quadratino striato se stiamo stampando l'ultima mappa, altrimenti stampo una cella generica bianca*/
            case 2:
                if(isTerminal)
                    printf("▨");
                else
                    printf("■");
                break;
            /* DEFAULT: errore o TOP_CELL se stiamo stampando l'ultima mappa, quadratino doppio */
            default:
                if(isTerminal)
                    printf("▣");
                else
                    printf("E");
                break;
            }
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("\n");
    }
}