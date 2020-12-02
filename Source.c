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

int x, y, SO_HEIGHT, SO_WIDTH;
int **map;

void init(int argc, char *argv[]);
void print_map(int isTerminal);

int main(int argc, char *argv[]){
    /* argv[0] = "Source", [1] = x, [2] = y, [3] = SO_HEIGHT, [4] = SO_WIDTH, [5] = NULL */
    if(argc != 6){
        fprintf(stderr, "\n%s: %d. WRONG NUMBER OF ARGUMENT RECEIVED :%d INSTEAD of 6!\n", __FILE__, __LINE__, argc);
	    exit(EXIT_FAILURE);
    }
    dprintf(1, "\nFIGLIO: %d\n",getpid());
    init(argc, argv);

    return(0);
}

void init(int argc, char *argv[]){
    x = atoi(argv[1]);
    y = atoi(argv[2]);

    SO_HEIGHT = atoi(argv[3]);
    SO_WIDTH = atoi(argv[4]);

    /*
    *map = shmat(atoi(argv[5]), 0, SHM_R);
    if(**map == -1)
        fprintf(stderr, "\n%s: %d. Errore nel reperire la memoria condivisa \n", __FILE__, __LINE__);*/
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
                printf("|X");
                break;
            /* CASO 1: cella di passaggio valida, non sorgente, quadratino bianco */
            case 1:
                printf("|_");
                break;
            /* CASO 2: cella sorgente, quadratino striato se stiamo stampando l'ultima mappa, altrimenti stampo una cella generica bianca*/
            case 2:
                if(isTerminal)
                    printf("|Z");
                else
                    printf("|_");
                break;
            /* DEFAULT: errore o TOP_CELL se stiamo stampando l'ultima mappa, quadratino doppio */
            default:
                if(isTerminal)
                    printf("|L");
                else
                    printf("E");
                break;
            }
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("|\n");
    }
}