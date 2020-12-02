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

int x, y, SO_HEIGHT, SO_WIDTH, alive = 1;
int **map;

void init(int argc, char *argv[]);
void print_map(int isTerminal);
void caller(int sig);
void killer(int sig);

int main(int argc, char *argv[]){
    struct sigaction nuova , vecchia;
    /* argv[0] = "Source", [1] = x, [2] = y, [3] = SO_HEIGHT, [4] = SO_WIDTH, [5] = NULL */
    if(argc != 6){
        fprintf(stderr, "\n%s: %d. WRONG NUMBER OF ARGUMENT RECEIVED :%d INSTEAD of 6!\n", __FILE__, __LINE__, argc);
	    exit(EXIT_FAILURE);
    }
    dprintf(1, "\nFIGLIO: %d\n",getpid());
    init(argc, argv);

    nuova.sa_handler = caller;
    sigemptyset(&nuova.sa_mask);
    nuova.sa_flags = 0;

    if (signal(SIGALRM, caller)==SIG_ERR) {
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }else if(signal(SIGQUIT, killer)==SIG_ERR){
        printf("\nErrore della disposizione dell'handler\n");
        exit(EXIT_FAILURE);
    }else{
        alarm(1);
        while(alive > 0){
            /* fa niente */
        }
    }
    return(0);
}

void init(int argc, char *argv[]){
    int memd;
    key_t key;

    x = atoi(argv[1]);
    y = atoi(argv[2]);

    SO_HEIGHT = atoi(argv[3]);
    SO_WIDTH = atoi(argv[4]);
    
    /* INIZIALIZZO LA SHARED MEMORY PER MAP *//*
    if ((key = ftok(".", 'b')) == -1)
    {
        perror("non esiste il file per la shared memory");
        exit(-1);
    }
    if((memd = shmget(key, sizeof(int**), SHM_R | SHM_W)) == -1) 
        fprintf(stderr, "\n%s: %d. Errore nella creazione della memoria condivisa\n", __FILE__, __LINE__);
    
    a = shmat(memd, 0, 0);
    if(a == (int*)(-1))
        fprintf(stderr, "\n%s: %d. Impossibile agganciare la memoria condivisa \n", __FILE__, __LINE__);

    dprintf(1,"memd = %d\n", memd);
    dprintf(1, "sh_map = %p\n",(void *) &a);
    dprintf(1, "a[0] = %d", a[0]);*/
}

void caller(int sig){
    int timing = 1;
    if(alive){
        dprintf(1,"Sono vivo eh! Sono : %d\n", getpid());
        alarm(timing);
    }
}

void killer(int sig){
    alive = 0;
    dprintf(1,"No vabbe io morta!\n");
}

void print_map(int isTerminal){
    /* indici per ciclare */
    int i, k;
    printf("stampa da figlio");
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