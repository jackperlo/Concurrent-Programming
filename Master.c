#include <stdio.h>

#include "Source.h"
#include "Taxi.h"


void map_generator(int **mappa, int SO_HOLES, int SO_SOURCES); 
/*
    genera la matrice con annesse celle HOLES e celle SO_SOURCES
*/

/*-------------COSTANTI GLOBALI-------------*/
const int SO_WIDTH; /* indica la larghezza della matrice che rappresenta la mappa */
const int SO_HEIGHT; /* indica l'altezza della matrice che rappresenta la mappa */
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
    
    return 0;
}

void map_generator(int **mappa, int SO_HOLES, int SO_SOURCES){

}