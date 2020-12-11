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

#define LOCK_RETURN s_queue_buff[0].sem_num = 2; s_queue_buff[0].sem_op = -1; s_queue_buff[0].sem_flg = 0; \
                    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
#define UNLOCK_RETURN s_queue_buff[0].sem_num = 2; s_queue_buff[0].sem_op = 1; s_queue_buff[0].sem_flg = 0; \
                    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}

int x, y; /* coordinate in cui si trova il taxi considerato */
int msg_queue_id = 0, trip_cell_counter=0; /* id del semaforo per la coda di messaggi */
int toX, toY; /* coordinate di destinazione taxi */
sigset_t masked, all; /* maschere per i segnali */ 
values_to_taxi *shd_mem_values_to_taxi; /* shd mem che mi passa i valori di map e so_timensec_map dal processo padre */
taxi_returned_values *aus_shd_mem_taxi_returned_values; /* struct locale utile per aggiornare in locale i valori dei parametri da ritornare: sarà copiata in mutex nella shd mem di ritorno alla morte del processo */
/* array di union configurato cosi: (vedi relativa union in common.h) */
taxi_returned_values *shd_mem_taxi_returned_values; /* shd mem dove aggiornerò i valori da ritornare una volta morto */
int trip_active=0; /*se c'è un viaggio in corso */

void init(int argc, char *argv[]); /* funzione di inizializzazione per le variabili globali al processo source e la mappa */
void struct_to_maps(); /* converte la struttura che contiene i valori delle mappe condivise passata dalla shd mem nelle due mappe locali relative: (int **map) (int **SO_TIMENSEC_MAP) */
void init_maps(); /* inizializza le mappa locali ai taxi e i parametri da ritornare */ 
void print_map(int isTerminal); /* stampa della mappa */
void signal_actions(); /* gestione dei segnali */ 
void signal_handler(int sig); /* handlers custom sui segnali gestiti*/
void print_map_specific(int** m, int isTerminal); /* stampa la mappa specificata come parametro */
void free_mat(); /* free dello spazio allocato con la malloc per le tre matrici locali */
void route_travel(int isToSource); /* gestisce il viaggio dei taxi e gestione dei suoi parametri */
int get_trip(); /* gestione della presa in carico di una richiesta */
int check_rcv_msg_status(int errn); /* check dell'avvenuta lettura da coda di messaggi */
int check_for_a_message_in_this_coordinates(int i, int j); /* metodo ausiliare per il check di un messaggio da parte di un source della coordinata passata come parametro */
void return_values(); /*metodo che scrive la memoria condivisa di ritorno al padre. Richiamata quando il processo termina*/
void wait_for_syncronization(); /*metodo d'appoggio che attende la sincronizzazione di tutti i taxi*/

int main(int argc, char *argv[]){
    /* controllo sul numero di parametri che il padre gli passa */
    if(argc != 9){ 
        fprintf(stderr, "\n%s: %d. ERRORE PASSAGGIO PARAMETRI.\nAspettati: 7\nRicevuti: %d\n", __FILE__, __LINE__, argc);
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* inizializzo la mappa che mi passa in shd mem il processo master */
    init(argc, argv); 
    
    /* inizializzo i segnali e i relativi handler che andranno a gestirli */
    signal_actions();

    /* attendo che tutti gli altri taxi siano pronti per eseguire */
    wait_for_syncronization();

    while (1)
    {
        if(!get_trip()){
            sleep(1);
        }
    }

    return(-1);
}

void init(int argc, char *argv[]){
    timeout.tv_sec = atoi(argv[6]); /*TIMEOUT DA SOSTITUIRE*/

    /* setto tutti i segnali come bloccati */
    sigfillset(&all); 

    x = atoi(argv[1]); /* coordinata x del processo corrente, passatagli dal master */
    y = atoi(argv[2]); /* coordinata y del processo corrente, passatagli dal master */

    srand((x*SO_WIDTH)+y);

    msg_queue_id = atoi(argv[4]); /* id coda di messaggi per parlare coi proc source */
    sem_sync_id = atoi(argv[5]); /* semaforo di sincronizzazione per lo start dell'esecuzione */
    sem_cells_id = atoi(argv[8]);

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
    sigaddset(&masked, SIGINT);
    abort.sa_mask = masked;
    abort.sa_handler = signal_handler;

    sigaction(SIGQUIT, &abort, NULL);
    sigaction(SIGINT, &abort, NULL);
}

void signal_handler(int sig){
    sigprocmask(SIG_BLOCK, &masked, NULL);
    switch (sig)
    {
        /* Termine Esecuzione del Taxi */
        case SIGQUIT:
            return_values();
            free_mat();
            if(trip_active)
                exit(TAXI_NOT_COMPLETED_STATUS); 
            else
                exit(0);
            break;
        
        case SIGINT:
            raise(SIGQUIT);
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

    SO_TIMENSEC_MAP = (long int **)malloc(SO_HEIGHT*sizeof(long int *));
    if (SO_TIMENSEC_MAP == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_TIMENSEC_MAP[i] = malloc(SO_WIDTH*sizeof(long int));
        if (SO_TIMENSEC_MAP[i] == NULL)
            return;
    }    

    /* inizializzazione struct ausiliaria per il ritorno. E' una struct LOCALE al taxi */
    aus_shd_mem_taxi_returned_values = (taxi_returned_values *)malloc((7 + (SO_HEIGHT*SO_WIDTH)) * sizeof(taxi_returned_values));
    if (aus_shd_mem_taxi_returned_values == NULL)
        return;
    bzero(aus_shd_mem_taxi_returned_values, (7 + (SO_HEIGHT*SO_WIDTH)) * sizeof(taxi_returned_values));

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
    long int *currentLongPtr;
    /* free map 2d array */
    for (i = 0; i < SO_HEIGHT; i++){
        currentIntPtr = map[i];
        free(currentIntPtr);

        currentLongPtr = SO_TIMENSEC_MAP[i];
        free(currentLongPtr);
    }
    free(aus_shd_mem_taxi_returned_values);
}

/* PER TEST DA CANCELLARE AL TERMINE */

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
                printf( BGRED KBLACK "|_|" RESET);
                break;
            /* CASO 1: cella di passaggio valida, non sorgente, quadratino bianco */
            case 1:
                printf("|_|" RESET);
                break;
            /* CASO 2: cella sorgente, quadratino striato se stiamo stampando l'ultima mappa, altrimenti stampo una cella generica bianca*/
            case 2:
                if(isTerminal)
                    printf( BGGREEN KBLACK "|_|" RESET);
                else
                    printf("|_|");
                break;
            /* DEFAULT: errore o TOP_CELL se stiamo stampando l'ultima mappa, quadratino doppio */
            default:
                if(isTerminal)
                    printf(BGMAGENTA "|_|" RESET);
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
    /* cicla per tutti gli elementi della mappa */
    for(i = 0; i < SO_HEIGHT; i++){
        for(k = 0; k < SO_WIDTH; k++){
            printf("|%d", m[i][k]);
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("|\n");
    }
}

int check_for_a_message_in_this_coordinates(int i, int j){
    int esito=0, data_size;
    LOCK_SIGNALS;
    LOCK_QUEUE;
    if(data_size = msgrcv(msg_queue_id, &msg_buffer, MSG_LEN, ((i*SO_WIDTH)+j)+1, IPC_NOWAIT)){
        if(data_size <= 0 && errno!=ENOMSG)
            dprintf(1, "ERORRE LETTURA MSGSND: %d", errno);
        else if(data_size > 0){
            esito = 1; /* ho trovato un messaggio in queste coordinate */
        }
    }else
        esito = 0; /* non ho trovato un messaggio in queste coordinate */
    UNLOCK_QUEUE;
    UNLOCK_SIGNALS; 
    
    if (esito)
    {
        toX = i;
        toY = j;
        route_travel(GOTO_SOURCE);
        toX = atoi(msg_buffer.mtext) / SO_WIDTH;
        toY = atoi(msg_buffer.mtext) % SO_WIDTH;
    }
    
    return esito;
}

int get_trip(){
    int ret_value=0, esito=0, errn, max=0, i=x, j=y, offset = 1, k, off_counter; /* cella di partenza: quella in cui si trova il taxi */
    
    /*
    propago la ricerca nelle celle piu vicine al Taxi 
         > > v
         ^ T v 
         ^ < <
    */
    /*calcolo del max*/
    if((SO_HEIGHT-i-1)>max) max = (SO_HEIGHT-i-1);
    if ((SO_WIDTH-j-1)>max) max = (SO_WIDTH-i-1);
    if ((i-0)>max) max = (i-0);
    if ((j-0)>max) max = (j-0);

    /* se ci sono richieste nella cella corrente del taxi prendo quelle */
    if(check_for_a_message_in_this_coordinates(i, j)){
        esito = 2;
    }else{
        while(esito != 2 && offset<=max){
            /*offset 1: 8 cella, offset 2: 16 celle, offset 3: 24 celle*/
            off_counter = offset * 8;
            for(k=0;k<off_counter;k++){
                if(k % 4 == 0){ /* lato alto */
                    if((x - offset) >= 0 && (y - offset + (k/4)) >= 0 && (y - offset + (k/4)) < SO_WIDTH ){
                        if(check_for_a_message_in_this_coordinates(x-offset, y-offset+(k/4))){
                            esito = 2;
                            break;
                        }
                    }
                }else if(k % 4 == 1){ /* lato destro */
                    if((y + offset) < SO_WIDTH && (x - offset + (k/4)) >= 0 && (x - offset + (k/4)) < SO_HEIGHT){
                        if(check_for_a_message_in_this_coordinates(x - offset + (k/4), y+offset)){
                            esito = 2;
                            break;
                        }
                    }
                }else if(k % 4 == 2){ /* lato sotto */
                    if((x + offset) < SO_HEIGHT && (y + offset - (k/4)) >= 0 && (y + offset - (k/4)) < SO_WIDTH){
                        if(check_for_a_message_in_this_coordinates(x + offset, y + offset - (k/4))){
                            esito = 2;
                            break;
                        }
                    }
                }else{ /* lato sinistro */
                    if((y - offset) >= 0 && (x + offset - (k/4)) >= 0 && (x + offset - (k/4)) < SO_WIDTH){
                        if(check_for_a_message_in_this_coordinates(x + offset - (k/4), y-offset)){
                            esito = 2;
                            break;
                        }
                    }
                }
            }
            offset++;   
        }
        
    }
    if(esito == 2){
        route_travel(0);
        return 1;
    }else
        return 0;
}

void route_travel(int isToSource){
    int possible = 0, action, time_tot_trip=0, crossed_cells=0, next, previous, old_position=0;
    struct timespec timer;
    timer.tv_sec = 0;
    trip_active=1;
    s_cells_buff[0].sem_op = 1; s_cells_buff[0].sem_flg = 0;
    s_cells_buff[1].sem_op = -1; s_cells_buff[1].sem_flg = 0;
    while(trip_active){
        s_cells_buff[0].sem_num = (x*SO_WIDTH)+y;
        if(x < toX && (((x+1)*SO_WIDTH)+y) != old_position){ /*prossima cella => basso */
            if(map[x+1][y] != 0)
                action = 0;                 
            else if((y+1)<SO_WIDTH && map[x][y+1] != 0)
                action = 2; 
            else if((y-1)>=0 && map[x][y-1] != 0)
                action = 3; 
        }else if(x > toX && (((x-1)*SO_WIDTH)+y) != old_position){ /*prossima cella => alto */
            if(map[x-1][y] != 0)
                action = 1;
            else if((y+1)<SO_WIDTH && map[x][y+1] != 0)
                action = 2; 
            else if((y-1)>=0 && map[x][y-1] != 0)
                action = 3;     
        }else if(y < toY && (x*SO_WIDTH)+(y+1) != old_position){ /*prossima cella => destra */
            if(map[x][y+1] != 0)
                action = 2;
            else if((x+1)<SO_HEIGHT && map[x+1][y] != 0)
                action = 0; 
            else if((x-1)>=0 && map[x-1][y] != 0)
                action = 1;  
        }else if(y > toY && (x*SO_WIDTH)+(y-1) != old_position){ /*prossima cella => sinistra */
            if(map[x][y-1] != 0)
                action = 3;
            else if((x+1)<SO_HEIGHT && map[x+1][y] != 0)
                action = 0; 
            else if((x-1)>=0 && map[x-1][y] != 0)
                action = 1;  
        }else if(y==toY && x==toX)
            trip_active=0;

        if(trip_active){
            old_position = (x*SO_WIDTH) + y;
            switch (action){
                case 0:
                    x++;
                    break;
                case 1:
                    x--;
                    break;
                case 2:
                    y++;
                    break;
                case 3:
                    y--;
                    break;
            }
            s_cells_buff[1].sem_num = (x*SO_WIDTH)+y;
            if(semtimedop(sem_cells_id, s_cells_buff, 2, &timeout) == -1){
                if(errno != EINTR && errno != EAGAIN){
                    TEST_ERROR;
                }else if(errno == EAGAIN){ /*gestione caso di deadlock per aver aspettato più di timeout*/
                    s_cells_buff[0].sem_num = (x*SO_WIDTH)+y; s_cells_buff[0].sem_op = 1;
                    while(semop(sem_cells_id, s_cells_buff, 1) == -1 && errno == EINTR);
                    free_mat();
                    exit(TAXI_ABORTED_STATUS);
                }else{/* reverses operation */
                    dprintf(1, KRED "\nREVERSE ACTION!\n" RESET);
                    switch (action){
                        case 0:
                            x--;
                            break;
                        case 1:
                            x++;
                            break;
                        case 2:
                            y--;
                            break;
                        case 3:
                            y++;
                            break;
                    }
                }
            }else{
                aus_shd_mem_taxi_returned_values[(x*SO_WIDTH)+y].cell_crossed_map_counter++;
                if(!isToSource){
                    time_tot_trip += SO_TIMENSEC_MAP[x][y];
                }
                timer.tv_nsec = SO_TIMENSEC_MAP[x][y];
                aus_shd_mem_taxi_returned_values[2].total_n_cells_crossed_value++;
                nanosleep(&timer, NULL);
            }
        }
    }
    if(!isToSource){
        aus_shd_mem_taxi_returned_values[0].completed_trips_counter++;
        aus_shd_mem_taxi_returned_values[3].max_trip_completed++;
        if(time_tot_trip > aus_shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value)
            aus_shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value = time_tot_trip;
    }    
}

int check_rcv_msg_status(int errn){
    switch (errno) {
	case EAGAIN:
		dprintf(STDERR_FILENO,
			"Queue is full and IPC_NOWAIT was set to have a non-blocking msgsnd()\nFix it by:\n(1) making sure that some process read messages, or\n(2) changing the queue size by msgctl()\n");
		return(-1);
	case EACCES:
		dprintf(STDERR_FILENO,
	        "No write permission to the queue.\nFix it by adding permissions properly\n");
		return(-1);
	case EFAULT:
		dprintf(STDERR_FILENO,
			"The address of the message isn't accessible\n");
		return(-1);
	case EIDRM:
		dprintf(STDERR_FILENO,
			"The queue was removed\n");
		return(-1);
	case EINTR:
		dprintf(STDERR_FILENO,
			"The process got unblocked by a signal, while waiting on a full queue\n");
		return(-1);
	case EINVAL:
		dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, __LINE__, getpid(), errno, strerror(errno));
		return(-1);
	case ENOMEM:
		dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, __LINE__, getpid(), errno, strerror(errno));
		return(-1);
    case ENOMSG:
        return(1);
	default:
		dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, __LINE__, getpid(), errno, strerror(errno));
	}
}

/*									0						1									2				             3	    	4			5		6	            	7		...*/				
/* taxi_returned_values = [completed_trips_counter, max_timensec_complete_trip_value, total_n_cells_crossed_value, max_trip_completed, pid [0], pid [1], pid [2], cell_crossed_map_counter[0], cell_crossed_map_counter[1], ...]; */
void return_values(){
    int i;
    LOCK_RETURN;
    shd_mem_taxi_returned_values[0].completed_trips_counter += aus_shd_mem_taxi_returned_values[0].completed_trips_counter;
    if(shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value < aus_shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value){
        shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value = aus_shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value;
        shd_mem_taxi_returned_values[4].pid = getpid();
    }
    if(shd_mem_taxi_returned_values[2].total_n_cells_crossed_value < aus_shd_mem_taxi_returned_values[2].total_n_cells_crossed_value){
        shd_mem_taxi_returned_values[2].total_n_cells_crossed_value = aus_shd_mem_taxi_returned_values[2].total_n_cells_crossed_value;
        shd_mem_taxi_returned_values[5].pid = getpid();
    }
    if(shd_mem_taxi_returned_values[3].max_trip_completed < aus_shd_mem_taxi_returned_values[3].max_trip_completed){
        shd_mem_taxi_returned_values[3].max_trip_completed = aus_shd_mem_taxi_returned_values[3].max_trip_completed;
        shd_mem_taxi_returned_values[6].pid = getpid();
    }
    for(i = 7; i < (7+(SO_WIDTH*SO_HEIGHT)); i++){
        shd_mem_taxi_returned_values[i].cell_crossed_map_counter += aus_shd_mem_taxi_returned_values[i].cell_crossed_map_counter;
    }
    UNLOCK_RETURN;
}

void wait_for_syncronization(){
    int sem_status = 1;
    sem_status = semctl(sem_sync_id, 1, GETVAL);
    /* attendo che tutti gli altri processi taxi siano pronti */
    if( sem_status > 0){
    s_queue_buff[0].sem_num = 1; 
    s_queue_buff[0].sem_op = -1; /* decrementa il semaforo di 1 */
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
    s_queue_buff[0].sem_num = 1;
    s_queue_buff[0].sem_op = 0; /* attende che il semaforo sia uguale a 0 */
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
    }
}