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

int x, y;
int timing = 1; /* inizializzazione tempo random dopo cui effettuare la prossima richiesta */
int requests = 0; /* numero di richieste effettuate da questo sorgente */
int msg_queue_id = 0; /* id del semaforo per la coda di messaggi */
sigset_t masked, all; /* maschere per i segnali */ 
values_to_source *shd_map; /* matrice sottoforma di array di stuct che contiene il solo valore della cella, allocata in shd mem e passata dal master */

void init(int argc, char *argv[]); /* funzione di inizializzazione per le variabili globali al processo source e la mappa */
void struct_to_map(); /* converte la struttura che contiene i valori della mappa condivisa passata dalla shd mem in una mappa locale (int **map) */
void init_map(); /* inizializza la mappa locale a source */
void signal_actions(); /* gestione dei segnali */ 
void signal_handler(int sig); /* handler custom sui segnali gestiti */
int generate_request();  /* metodo di supporto che genera e inserisce la richiesta nella coda di messaggi, con gestione relativi semafori di mutua esclusione */
int check_snd_msg_status(int errn); /* controlla l'esito di una message send */
void free_mat(); /* libero lo spazio allocato per la matrice map locale */
void check_map_to_allow_requests(); /* controlla che nella mappa esista almeno una cella non source e non inaccessibile (che possa quindi essere scelta da destinazione per le richieste) */
void wait_for_syncronization(); /*metodo d'appoggio che attende la sincronizzazione di tutti i source*/

int main(int argc, char *argv[]){
    /* controllo sul numero di parametri che il padre gli passa */
    if(argc != 6){ 
        fprintf(stderr, "\n%s: %d. ERRORE PASSAGGIO PARAMETRI.\nAspettati: 6\nRicevuti: %d\n", __FILE__, __LINE__, argc);
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* inizializzo la mappa che mi passa in shd mem il processo master */
    init(argc, argv); 
    
    /* inizializzo i segnali e i relativi handler che andranno a gestirli */
    signal_actions();

    /* controlla che nella mappa esista almeno una cella non source e non inaccessibile (che possa quindi essere scelta da destinazione per le richieste)*/
    check_map_to_allow_requests();

    /* attendo che tutti gli altri taxi siano pronti per eseguire */
    wait_for_syncronization();

    /* INIZIO ESECUZIONE DEI SOURCE */

    /* parte un contatore casuale che emetterà una richiesta di taxi */
    raise(SIGALRM);
    
    /* continuo finché non mi arriva un segnale che termina l'esecuzione */
    while (1)
    {
        pause();
    }

    return(-1); /* -1 errore nella chiusura di source, altrimenti restituisce il numero di richieste effettuate */
}

void init(int argc, char *argv[]){
    int rand_seed;

    sigfillset(&all);

    x = atoi(argv[1]); /* coordinata x del processo corrente, passatagli dal master */
    y = atoi(argv[2]); /* coordinata y del processo corrente, passatagli dal master */

    rand_seed = (x*SO_WIDTH)+y;
    srand(rand_seed); /* inizializzo il random per i futuri segnali di alarm con un seed univoco per ogni processo */

    msg_queue_id = atoi(argv[4]);
    sem_sync_id = atoi(argv[5]);

    if((SO_WIDTH <= 0) || (SO_HEIGHT <= 0)){
        fprintf(stderr, "\nPARAMETRI DI COMPILAZIONE INVALIDI. SO_WIDTH o SO_HEIGHT <= 0.");
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* attacco source alla shd mem aperta dal padre */
    shd_map = (values_to_source *)shmat(atoi(argv[3]), NULL, 0);
    if(shd_map == (values_to_source *)(-1))
        fprintf(stderr, "\n%s: %d. Impossibile agganciare la memoria condivisa", __FILE__, __LINE__);

    init_map();
}

void signal_actions(){
    /* struct per mascherare i segnali con diversi comportamenti */
    struct sigaction restart, abort;

    /* pulisce le struct impostando tutti i bytes a 0*/
    bzero(&restart, sizeof(restart)); 
    bzero(&abort, sizeof(abort));

    /* assegno l'handler ad ogni maschera */
    restart.sa_handler = signal_handler;
    abort.sa_handler = signal_handler;

    /* flags per comportamenti speciali assunti dalla maschera */ 
    restart.sa_flags = SA_RESTART;
    abort.sa_flags = 0; /* nessun comportamento speciale */

    sigaddset(&masked, SIGALRM);
    sigaddset(&masked, SIGQUIT);
    sigaddset(&masked, SIGINT);
    sigaddset(&masked, SIGUSR1); /* sfrutto la maschera con flag di restart già usata per il SIGARLM*/

    restart.sa_mask = masked;
    abort.sa_mask = masked;

    if(sigaction(SIGALRM, &restart, NULL) || sigaction(SIGQUIT, &abort, NULL) || sigaction(SIGUSR1, &restart, NULL) || sigaction(SIGINT, &abort, NULL)){
        exit(EXIT_FAILURE_CUSTOM);
    }
}

void signal_handler(int sig){
    sigprocmask(SIG_BLOCK, &masked, NULL);
    switch (sig)
    {
        /* Generazione Richiesta di Taxi */
        case SIGALRM:
            requests++;
            generate_request();
            /* dprintf(1,"\nAggiungo una richiesta in coda! Sono : %d", getpid());*/
            /* prossima richiestra verrà fatta tra timing secondi.. */ 
            timing = 1 + rand() % 10; 
            alarm(timing);
            break;

        /* Termine Esecuzione del Source */
        case SIGQUIT:
            alarm(0); /* RESET DELL'ALARM. Cosi non si genererà più la richiesta che stava avanzando dalla chiamata all'ultimo alarm */
            /* RITORNO AL PADRE IL NUMERO DI RICHIESTE CHE HO ESEGUITO */
            free_mat();
            exit(requests); 
            break;
        case SIGINT:
            raise(SIGQUIT);
            break;
        /* richiesta esplicita da terminale giunta dal processo master (solo il figlio con uno specifico PID scelto dal padre triggererà questo handler) */
        case SIGUSR1:
            requests++;
            generate_request();
            /*dprintf(1,"\nAGGIUNGO LA RICHIESTA ESPLICITA DA TERMINALE IN CODA! Sono : %d", getpid());*/
            break;

        default:
            dprintf(1, "\nSignal %d not handled\n", sig);
            break;
    }
    sigprocmask(SIG_UNBLOCK, &masked, NULL);
}

void init_map(){
    int i, j;

    map = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (map == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        map[i] = malloc(SO_WIDTH*sizeof(int));
        if (map[i] == NULL)
            return;
    }

    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++)
            map[i][j] = 1; /* rendo ogni cella vergine(no sorgente, no inaccessibile) */
    }
    
    struct_to_map(); /* conversione da shared memory a matrice map */
}

void struct_to_map(){
    int i, k, cnt=0;
    for (i = 0; i < SO_HEIGHT; i++){
        for(k= 0; k < SO_WIDTH; k++){
            map[i][k] = shd_map[cnt].cell_map_value; 
            cnt++;
        }
    }
    shmdt(shd_map);
}

int generate_request(){
    int toX, toY, errn, esito; /* coordinate di arrivo della richiesta effettuata da source*/ 
    do
    {
        toX = rand() % SO_HEIGHT;
        toY = rand() % SO_WIDTH;
    }while(map[toX][toY] != 1);
   
    /* inizializzo i parametri nel buffer */
    msg_buffer.mtype = (x * SO_WIDTH) + y + 1;
    if (msg_buffer.mtype <= 0) {
		fprintf(stderr, "\n%s: %d. parametro mtype di una coda di messaggi deve essere > 0. Tentato inserimento di: %d\n", __FILE__, __LINE__, ((x * SO_WIDTH) + y));
		return(-1);
	}
    sprintf(msg_buffer.mtext, "%d", (toX * SO_WIDTH) + toY);;

    LOCK_SIGNALS;
    LOCK_QUEUE;
    
    if(errn = msgsnd(msg_queue_id, &msg_buffer, MSG_LEN, 0)){
        if((esito = check_snd_msg_status(errn)) == -1)
            exit(esito);
    }
    UNLOCK_QUEUE;
    UNLOCK_SIGNALS;
}

void free_mat(){
    int i;
    int *currentIntPtr;
    /* free map 2d array */
    for (i = 0; i < SO_HEIGHT; i++){
        currentIntPtr = map[i];
        free(currentIntPtr);
    }
}

int check_snd_msg_status(int errn){
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
	default:
		dprintf(STDERR_FILENO, "%s:%d: PID=%5d: Error %d (%s)\n", __FILE__, __LINE__, getpid(), errno, strerror(errno));
	}
}

void check_map_to_allow_requests(){
    int i, j, normal_cells_counter=0;
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++){
            if(map[i][j] == 1)
                normal_cells_counter++;
        }
    }
    if(normal_cells_counter == 0){
        dprintf(1, "\nSource %d: no destinazioni possibili.\n", getpid());
        /* attendo che tutti gli altri processi source siano pronti */
        s_queue_buff[0].sem_num = 1; 
        s_queue_buff[0].sem_op = -1; /* decrementa il semaforo di 1 */
        s_queue_buff[0].sem_flg = 0;
        while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
        raise(SIGQUIT);
    }    
}

void wait_for_syncronization(){
    /* attendo che tutti gli altri processi source siano pronti */
    s_queue_buff[0].sem_num = 1; 
    s_queue_buff[0].sem_op = -1; /* decrementa il semaforo di 1 */
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
    s_queue_buff[0].sem_num = 1;
    s_queue_buff[0].sem_op = 0; /* attende che il semaforo sia uguale a 0 */
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
}