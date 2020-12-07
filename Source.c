#include "Common.h"

#define CLEAN

#define EXIT_FAILURE_CUSTOM -1

#define LOCK_SIGNALS sigprocmask(SIG_BLOCK, &all, NULL);
#define UNLOCK_SIGNALS sigprocmask(SIG_UNBLOCK, &all, NULL);

#define LOCK_QUEUE s_queue_buff[0].sem_num = 0; s_queue_buff[0].sem_op = -1; s_queue_buff[0].sem_flg = 0; \
                    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
#define UNLOCK_QUEUE s_queue_buff[0].sem_num = 0; s_queue_buff[0].sem_op = 1; s_queue_buff[0].sem_flg = 0; \
                    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}

int x, y, timing = 1, requests = 0, msg_queue_id = 0;
sigset_t masked, all, tipo; /* maschere per i segnali */ 
mapping *shd_map;

void init(int argc, char *argv[]); /* funzione di inizializzazione per le variabili globali al processo source e la mappa */
void struct_to_map(); /* converte la struttura che contiene i valori della mappa condivisa passata dalla shd mem in una mappa locale (int **map) */
void init_map(); /* inizializza la mappa locale a source */
void signal_actions(); /* */ 
void signal_handler(int sig); /* */
int generate_request(); 
int check_snd_msg_status(int errn); /* controlla l'esito di una message send */

int main(int argc, char *argv[]){
    int sig = SIGQUIT;
    /* controllo sul numero di parametri che il padre gli passa */
    if(argc != 6){ 
        fprintf(stderr, "\n%s: %d. ERRORE PASSAGGIO PARAMETRI.\nAspettati: 4\nRicevuti: %d\n", __FILE__, __LINE__, argc);
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* inizializzo la mappa che mi passa in shd mem il processo master */
    init(argc, argv); 
    
    /* inizializzo i segnali e i relativi handler che andranno a gestirli */
    signal_actions();

    /* attendo che tutti gli altri processi source siano pronti */
    s_queue_buff[0].sem_num = 1; 
    s_queue_buff[0].sem_op = -1; 
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
    s_queue_buff[0].sem_num = 1;
    s_queue_buff[0].sem_op = 0; 
    s_queue_buff[0].sem_flg = 0;
    while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}

    /* parte un contatore casuale che emetterà una richiesta di taxi */
    raise(SIGALRM);
    
    /* INIZIO ESECUZIONE DEI SOURCE */
    /* 
    un source rimane sempre in pausa fino a quando l'alarm (di un tempo casuale) scade e risveglia
    il processo che invierà una richiesta di taxi 

    sigwait permette di attendere finche non arriva la SIGQUIT che termina il processo, ma la return dopo sigwait() non deve mai essere raggiunta
    */
    sigaddset(&tipo, SIGQUIT);
    sigwait(&tipo, &sig);

    return(-1);
}

void init(int argc, char *argv[]){
    char *s;
    int rand_seed;

    sigfillset(&all);

    x = atoi(argv[1]); /* coordinata x del processo corrente, passatagli dal master */
    y = atoi(argv[2]); /* coordinata y del processo corrente, passatagli dal master */

    rand_seed = (x*SO_WIDTH)+y;
    srand(rand_seed); /* inizializzo il random per i futuri segnali di alarm con un seed univoco per ogni processo */

    msg_queue_id = atoi(argv[4]);
    sem_sync_id = atoi(argv[5]);

    if((SO_WIDTH <= 0) || (SO_HEIGHT <= 0)){
        fprintf(stderr, "PARAMETRI DI COMPILAZIONE INVALIDI. SO_WIDTH o SO_HEIGHT <= 0.\n");
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* attacco source alla shd mem aperta dal padre */
    shd_map = (mapping*)shmat(atoi(argv[3]), NULL, 0);
    if(shd_map == (mapping*)(-1))
        fprintf(stderr, "\n%s: %d. Impossibile agganciare la memoria condivisa \n", __FILE__, __LINE__);

    init_map();
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
            dprintf(1,"Aggiungo una richiesta in coda! Sono : %d\n", getpid());
            generate_request();
            /* prossima richiestra verrà fatta tra timing secondi.. */ 
            timing = 1 + rand() % 10; 
            alarm(timing);
            break;

        /* Termine Esecuzione del Source */
        case SIGQUIT:
            alarm(0); /* RESET DELL'ALARM. Cosi non si genererà più la richiesta che stava avanzando dalla chiamata all'ultimo alarm */
            dprintf(1,"Fine esecuzione processo source!\n"); /* da togliere */ 
            /* RITORNO AL PADRE IL NUMERO DI RICHIESTE CHE HO ESEGUITO */
            exit(requests); 
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
            map[i][k] = shd_map[cnt].value; 
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
    }while(map[toX][toY] == 0);

    /* inizializzo i parametri nel buffer */
    msg_buffer.mtype = (x * SO_WIDTH) + y + 1;
    if (msg_buffer.mtype <= 0) {
		fprintf(stderr, "\n%s: %d. parametro mtype di una coda di messaggi deve essere > 0. Tentato inserimento di: %d\n", __FILE__, __LINE__, ((x * SO_WIDTH) + y));
		return(-1);
	}
    sprintf(msg_buffer.mtext, "%d", (toX * SO_WIDTH) + toY);

    LOCK_SIGNALS;
    LOCK_QUEUE;
    if(errn = msgsnd(msg_queue_id, &msg_buffer, MSG_LEN, 0)){
        if((esito = check_snd_msg_status(errn)) == -1)
            exit(esito);
    }
    UNLOCK_QUEUE;
    UNLOCK_SIGNALS;
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