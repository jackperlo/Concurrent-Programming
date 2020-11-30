#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Source.h"
#include "Taxi.h"

/*-------------DEFINE di COSTANTI--------------*/
#ifndef SO_HEIGHT
#define SO_HEIGHT 10 /* indica l'altezza della matrice che rappresenta la mappa, se non è definito imposto un valore arbitrario di 10 */
#endif

#ifndef SO_WIDTH
#define SO_WIDTH 10 /* indica la larghezza della matrice che rappresenta la mappa, se non è definito imposto un valore arbitrario di 10 */
#endif

/*------------DICHIARAZIONE METODI------------*/
void map_generator(); /* genera la matrice con annesse celle HOLES e celle SO_SOURCES */
void init_map(); /* inizializza la matrice vergine (tutte le celle a 1)*/
int check_cell_2be_inaccessible(int x, int y); /* metodo di supporto a map_generator(). controlla se le 8 celle adiacenti a quella considerata sono tutte non inaccessibili */ 
void print_map(int isTerminal); /* stampa una vista della mappa durante l'esecuzione, e con isTerminal evidenzia le SO_TOP_CELLS celle con più frequenza di passaggio */

/*-------------COSTANTI GLOBALI-------------*/
const int SO_TAXI; /* numero di taxi presenti nella sessione in esecuzione */
const int SO_CAP_MIN; /* capacità minima assumibile da SO_CAP: determina il minimo numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
const int SO_CAP_MAX; /* capacità massima assumibile da SO_CAP: determina il MASSIMO numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
const int SO_TIMESEC_MIN; /* valore minimo assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */
const int SO_TIMESEC_MAX; /* valore MASSIMO assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */

/*-------------VARIABILI GLOBALI-------------*/
int map[SO_HEIGHT][SO_WIDTH]; /* matrice che determina la mappa in esecuzione */

int SO_HOLES; /* indica il numero di celle inaccessibili(da cui holes) all'interno della matrice */ 
int SO_SOURCES; /* indica il numero di celle sorgenti di possibili richieste. Come fossero le stazioni di servizio per i taxi */

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
    SO_HOLES = atoi(argv[1]); /* assumo che il primo parametro del main sia il numero di holes */
    SO_SOURCES = atoi(argv[2]); /* secondo parametro del main -> numero di sorgenti nella mappa */

    map_generator();
    print_map(0);
    
    return 0;
}

void map_generator(){
    int i, x, y;
    int esito=0; /* valore restituito dalla check_cell_2be_inaccessible(): 0 -> cella non adatta ad essere inaccessibile per vincoli di progetto. 1 -> cella adatta ad essere inaccessibile */
    
    init_map();
    srand(time(NULL)); /* inizializzo il random number generator */
    for (i = 0; i < SO_HOLES; i++){
        do{
            x = rand() % SO_HEIGHT; /* estrae un random tra 0 e (SO_HEIGHT-1) */
            y = rand() % SO_WIDTH; /* estrae un random tra 0 e (SO_WIDTH-1) */
            if(map[x][y] != 0) /* se la cella non è già segnata come inaccessibile */
                esito = check_cell_2be_inaccessible(x, y);
        }while(esito == 0); /* finché non trovo una cella adatta ad essere definita inaccessibile */
        map[x][y] = 0; /* rendo effettivamente la cella inaccessibile */
        esito = 0; 
    }
}

void init_map(){
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++)
            map[i][j] = 1; /* rendo ogni cella vergine(no sorgente, no inaccessibile) */
    }
}

int check_cell_2be_inaccessible(int x, int y){ 
    int esito = 1; /* assumo che sia possibile rendere la cella inaccessibile */
    /* devo controllare che tutte le celle X siano (!= 0), dove 0 => cella inaccessibile
        X X X
        X o X
        X X X
    */

    if((x-1) >= 0){ /* check sull'INTERA RIGA SOPRA alla cella considerata, se non fuori dai bordi */
        if((y-1) >= 0){
            if(map[x-1][y-1] == 0) /* è già inaccessibile => CELLA NON UTILE */
                esito = 0;
        }

        if(map[x-1][y] == 0)
            esito = 0;

        if((y+1) < SO_WIDTH){
            if(map[x-1][y+1] == 0)
                esito = 0;
        }
    }

    if((esito == 1) && ((x+1) < SO_HEIGHT)){ /* check sull'INTERA RIGA SOTTO, SE la RIGA SOPRA era OK(esito rimasto 1)*/
        if((y-1) >= 0){
            if(map[x+1][y-1] == 0)
                esito = 0;
        }

        if(map[x+1][y] == 0)
            esito = 0;

        if((y+1) < SO_WIDTH){
            if(map[x+1][y+1] == 0)
                esito = 0;
        }
    }

    if((esito == 1) && ((y-1) >= 0)){ /* CELLA a SINISTRA */
        if(map[x][y-1] == 0)
            esito = 0;
    }

    if((esito == 1) && ((y+1) < SO_WIDTH)){ /* CELLA a DESTRA */
        if(map[x][y+1] == 0)
            esito = 0;
    } 

    return esito;
}

void print_map(int isTerminal){
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