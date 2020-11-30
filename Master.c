#include <stdio.h>

#include "Source.h"
#include "Taxi.h"


    void map_generator(int *mappa[][], int SO_HOLES, int SO_SOURCES); 
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
const int SO_TIMESEC_MIN; /* valore MASSIMO assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */

/*-------------VARIABILI GLOBALI-------------*/

int main(int argc, char *argv[]){
    
    return 0;
}