#include "Common.h"

/* define richiamata dalla macro TESTERROR definita in common */
#define CLEAN

#define EXIT_FAILURE_CUSTOM -1

/* blocca e sblocca tutti i segnali */
#define LOCK_SIGNALS sigprocmask(SIG_BLOCK, &all, NULL);
#define UNLOCK_SIGNALS sigprocmask(SIG_UNBLOCK, &all, NULL);

/* semaforo di mutua esclusione sulla scrittura in coda di messaggi */
#define LOCK_QUEUE s_queue_buff[0].sem_num = 0; s_queue_buff[0].sem_op = -1; s_queue_buff[0].sem_flg = 0; \
                    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
#define UNLOCK_QUEUE s_queue_buff[0].sem_num = 0; s_queue_buff[0].sem_op = 1; s_queue_buff[0].sem_flg = 0; \
                    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}

int x, y; /* coordinate in cui si trova il taxi considerato */
int msg_queue_id = 0; /* id del semaforo per la coda di messaggi */
int toX, toY; /* coordinate di destinazione taxi */
sigset_t masked, all; /* maschere per i segnali */ 
values_to_taxi *shd_mem_values_to_taxi; /* shd mem che mi passa i valori di map e so_timensec_map dal processo padre */
taxi_returned_values *aus_shd_mem_taxi_returned_values; /* struct locale utile per aggiornare in locale i valori dei parametri da ritornare: sarà copiata in mutex nella shd mem di ritorno alla morte del processo */
/* array di union configurato cosi: */
/* taxi_returned_values = [completed_trips_counter, max_timensec_complete_trip_value, max_cells_crossed_longest_trip_value, cell_crossed_map_counter[0], cell_crossed_map_counter[1], ...]; */
taxi_returned_values *shd_mem_taxi_returned_values; /* shd mem dove aggiornerò i valori da ritornare una volta morto */

void init(int argc, char *argv[]); /* funzione di inizializzazione per le variabili globali al processo source e la mappa */
void struct_to_maps(); /* converte la struttura che contiene i valori delle mappe condivise passata dalla shd mem nelle due mappe locali relative: (int **map) (int **SO_TIMENSEC_MAP) */
void init_maps(); /* inizializza le mappa locali ai taxi e i parametri da ritornare */ 
void print_map(int isTerminal); /* stampa della mappa */
void signal_actions(); /* gestione dei segnali */ 
void signal_handler(int sig); /* handlers custom sui segnali gestiti*/
void print_map_specific(int** m, int isTerminal); /* stampa la mappa specificata come parametro */
void free_mat(); /* free dello spazio allocato con la malloc per le tre matrici locali */

int main(int argc, char *argv[]){
    /* controllo sul numero di parametri che il padre gli passa */
    if(argc != 8){ 
        fprintf(stderr, "\n%s: %d. ERRORE PASSAGGIO PARAMETRI.\nAspettati: 7\nRicevuti: %d\n", __FILE__, __LINE__, argc);
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* inizializzo la mappa che mi passa in shd mem il processo master */
    init(argc, argv); 
    
    /* inizializzo i segnali e i relativi handler che andranno a gestirli */
    signal_actions();

    /* attendo che tutti gli altri processi source siano pronti */
    /*s_queue_buff[0].sem_num = 1; 
    s_queue_buff[0].sem_op = -1; 
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
    s_queue_buff[0].sem_num = 1;
    s_queue_buff[0].sem_op = 0; 
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}*/

    /* parte un contatore casuale che emetterà una richiesta di taxi */
    /*raise(SIGALRM);*/
    
    /* INIZIO ESECUZIONE DEI SOURCE */
    /* 
    un source rimane sempre in pausa fino a quando l'alarm (di un tempo casuale) scade e risveglia
    il processo che invierà una richiesta di taxi 

    sigwait permette di attendere finche non arriva la SIGQUIT che termina il processo, ma la return dopo sigwait() non deve mai essere raggiunta
    */

    while (1)
    {
        pause();
    }

    return(-1);
}

void init(int argc, char *argv[]){
    /* setto tutti i segnali come bloccati */
    sigfillset(&all); 

    x = atoi(argv[1]); /* coordinata x del processo corrente, passatagli dal master */
    y = atoi(argv[2]); /* coordinata y del processo corrente, passatagli dal master */

    msg_queue_id = atoi(argv[4]); /* id coda di messaggi per parlare coi proc source */
    sem_sync_id = atoi(argv[5]); /* semaforo di sincronizzazione per lo start dell'esecuzione */

    if((SO_WIDTH <= 0) || (SO_HEIGHT <= 0)){
        fprintf(stderr, "PARAMETRI DI COMPILAZIONE INVALIDI. SO_WIDTH o SO_HEIGHT <= 0.\n");
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* attacco taxi alla shd mem per i valori da ritornare al padre */
    shd_mem_taxi_returned_values =shmat(atoi(argv[7]), NULL, 0);
    if(shd_mem_taxi_returned_values == (taxi_returned_values *)(-1))
        fprintf(stderr, "\n%s: %d. Impossibile agganciare la memoria condivisa \n", __FILE__, __LINE__);

    /* attacco taxi alla shd mem per i valori passati dal padre al taxi */
    shd_mem_values_to_taxi = shmat(atoi(argv[3]), NULL, 0);
    if(shd_mem_values_to_taxi == (values_to_taxi *)(-1))
        fprintf(stderr, "\n%s: %d. Impossibile agganciare la memoria condivisa \n", __FILE__, __LINE__);

    init_maps();
   
    shmdt(shd_mem_values_to_taxi);
}

void signal_actions(){
    struct sigaction abort;

    /* pulisce le struct */
    bzero(&abort, sizeof(abort));
    sigaddset(&masked, SIGQUIT);
    abort.sa_mask = masked;
    abort.sa_handler = signal_handler;

    sigaction(SIGQUIT, &abort, NULL);
}

void signal_handler(int sig){
    sigprocmask(SIG_BLOCK, &masked, NULL);
    switch (sig)
    {
        /* Termine Esecuzione del Taxi */
        case SIGQUIT:
            dprintf(1,"Fine esecuzione processo taxi %d!\n", getpid()); /* da togliere */ 
            free_mat();
            exit(TAXI_NOT_COMPLETED_STATUS); 
            break;

        default:
            dprintf(1, "\nSignal %d not handled\n", sig);
            break;
    }
    sigprocmask(SIG_UNBLOCK, &masked, NULL);
}

void init_maps(){
    int i, j;

    map = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (map == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        map[i] = malloc(SO_WIDTH*sizeof(int));
        if (map[i] == NULL)
            return;
    }

    SO_TIMENSEC_MAP = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (SO_TIMENSEC_MAP == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_TIMENSEC_MAP[i] = malloc(SO_WIDTH*sizeof(int));
        if (SO_TIMENSEC_MAP[i] == NULL)
            return;
    }    

    /* inizializzazione struct ausiliaria per il ritorno. E' una struct LOCALE al taxi */
    aus_shd_mem_taxi_returned_values = (taxi_returned_values *)malloc((3 + (SO_HEIGHT*SO_WIDTH)) * sizeof(taxi_returned_values));
    if (aus_shd_mem_taxi_returned_values == NULL)
        return;
    bzero(aus_shd_mem_taxi_returned_values, (3 + (SO_HEIGHT*SO_WIDTH)) * sizeof(taxi_returned_values));

    /* conversione da shared memory a matrice map e matrice so_timensec_map */
    struct_to_maps(); 
}

void struct_to_maps(){
    int i, k, cnt=0;
    for (i = 0; i < SO_HEIGHT; i++){
        for(k= 0; k < SO_WIDTH; k++){
            map[i][k] = (shd_mem_values_to_taxi+cnt)->cell_map_value;
            SO_TIMENSEC_MAP[i][k] = (shd_mem_values_to_taxi+cnt)->cell_timensec_map_value;
            cnt++;
        }
    }
}

void free_mat(){
    int i;
    int *currentIntPtr;
    /* free map 2d array */
    for (i = 0; i < SO_HEIGHT; i++){
        currentIntPtr = map[i];
        free(currentIntPtr);

        currentIntPtr = SO_TIMENSEC_MAP[i];
        free(currentIntPtr);
    }
    free(aus_shd_mem_taxi_returned_values);
}

/* PER TEST DA CANCELLARE AL TERMINE */

void print_map(int isTerminal){
    /* indici per ciclare */
    int i, k;
    printf("stampa da figlio taxi\n");
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
                    printf("|%c",(char)map[i][k]);
                break;
            }
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("|\n");
    }
}

void print_map_specific(int** m, int isTerminal){
    /* indici per ciclare */
    int i, k;
    /*printf("value = %p",(void *) &m);*/
    printf("Specifica map:\n");
    /* cicla per tutti gli elementi della mappa */
    for(i = 0; i < SO_HEIGHT; i++){
        for(k = 0; k < SO_WIDTH; k++){
            printf("|%d", m[i][k]);
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("|\n");
    }
}