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

int x, y, SO_HEIGHT, SO_WIDTH, timing = 1, requests = 0;
int **map;
sigset_t masked;

void init(int argc, char *argv[]);
void print_map(int isTerminal);
void caller(int sig);
void killer(int sig);
int stampaStatoMemoria(int shid);
void cpy_map(int begin, char *argv[]);
void signal_actions();
void signal_handler(int sig);

int main(int argc, char *argv[]){
    
    init(argc, argv);
    print_map(0);
    
    signal_actions();
    alarm(1+rand()%10);
    
    while (1)
    {
        pause();
    }
    
    return(0);
}

void init(int argc, char *argv[]){
    int memd, i;
    key_t key;

    srand(time(NULL));

    x = atoi(argv[1]);
    y = atoi(argv[2]);

    SO_HEIGHT = atoi(argv[3]);
    SO_WIDTH = atoi(argv[4]);

    map = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (map == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        map[i] = malloc(SO_WIDTH*sizeof(int));
        if (map[i] == NULL)
            return;
    }

    cpy_map(5, argv);
}

void cpy_map(int begin, char *argv[]){
    int i, k;
    for(i = 0; i < SO_HEIGHT; i++){
        for(k = 0; k < SO_WIDTH; k++){
            map[i][k] = argv[begin+i][k];
        }
    }
}

void signal_actions(){
    struct sigaction restart, abort; /* nodefer flag, defer flag */

    /* pulisce le struct */
    bzero(&restart, sizeof(restart)); 
    bzero(&abort, sizeof(abort));

    restart.sa_handler = signal_handler;
    abort.sa_handler = signal_handler;

    restart.sa_flags = SA_RESTART; /* permette invocazioni innestate dell'handler */

    sigaddset(&masked, SIGALRM);
    sigaddset(&masked, SIGQUIT);

    restart.sa_mask = masked;
    abort.sa_mask = masked;

    if(sigaction(SIGALRM, &restart, NULL) || sigaction(SIGQUIT, &abort, NULL)){
        exit(EXIT_FAILURE);
    }
}

void signal_handler(int sig){
    sigprocmask(SIG_BLOCK, &masked, NULL);
    switch (sig)
    {
        /* Generazione Richiesta */
        case SIGALRM:
            dprintf(1,"Aggiungo una richiesta in coda! Sono : %d\n", getpid());
            timing = 1 + rand() % 10;
            alarm(timing);
            break;

        /* Termine Esecuzione */
        case SIGQUIT:
            alarm(0);
            dprintf(1,"Fine esecuzione processo source!\n");
            exit(requests);
            break;

        default:
            dprintf(1, "\nSignal %d not handled\n", sig);
            break;
    }
    sigprocmask(SIG_UNBLOCK, &masked, NULL);
}

void print_map(int isTerminal){
    /* indici per ciclare */
    int i, k;
    printf("stampa da figlio\n");
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
                    printf("|E");
                break;
            }
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("|\n");
    }
}