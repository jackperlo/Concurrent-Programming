#include "Common.h"

#define EXIT_FAILURE_CUSTOM -1
#define LOCK_QUEUE if(sem_queue = semget(IPC_PRIVATE, 1, 0)){fprintf(stderr, "\n%s: %d. Errore semget queue source pid: %d\n", __FILE__, __LINE__, getpid());exit(EXIT_FAILURE_CUSTOM);} \
                    s_queue_buff.sem_num = 0; s_queue_buff.sem_op = -1; s_queue_buff.sem_flg = 0; \
                    if(semop(sem_queue, &s_queue_buff, 1)){fprintf(stderr, "\n%s: %d. Errore semop queue source pid: %d\n", __FILE__, __LINE__, getpid());exit(EXIT_FAILURE_CUSTOM);}

#define UNLOCK_QUEUE if(sem_queue = semget(IPC_PRIVATE, 1, 0)){fprintf(stderr, "\n%s: %d. Errore semget queue source pid: %d\n", __FILE__, __LINE__, getpid());exit(EXIT_FAILURE_CUSTOM);} \
                    s_queue_buff.sem_num = 0; s_queue_buff.sem_op = 1; s_queue_buff.sem_flg = 0; \
                    if(semop(sem_queue, &s_queue_buff, 1)){fprintf(stderr, "\n%s: %d. Errore semop queue source pid: %d\n", __FILE__, __LINE__, getpid());exit(EXIT_FAILURE_CUSTOM);}

int x, y, timing = 1, requests = 0, msq_id;
sigset_t masked; /* maschera per i segnali */ 
mapping *shd_map;

void init(int argc, char *argv[]); /* funzione di inizializzazione per le variabili globali al processo source e la mappa */
void struct_to_map(); /* converte la struttura che contiene i valori della mappa condivisa passata dalla shd mem in una mappa locale (int **map) */
void init_map(); /* inizializza la mappa locale a source */ 
void print_map(int isTerminal); /* stampa della mappa */
void signal_actions(); /* */ 
void signal_handler(int sig); /* */
int generate_request(); 
int check_snd_msg_status(int errn); /* controlla l'esito di una message send */

int main(int argc, char *argv[]){
    /* controllo sul numero di parametri che il padre gli passa */
    if(argc != 4){ 
        fprintf(stderr, "\n%s: %d. ERRORE PASSAGGIO PARAMETRI.\nAspettati: 4\nRicevuti: %d\n", __FILE__, __LINE__, argc);
		exit(EXIT_FAILURE_CUSTOM);
    }

    /* inizializzo la mappa che mi passa in shd mem il processo master */
    init(argc, argv); 
    
    /* inizializzo i segnali e i relativi handler che andranno a gestirli */
    signal_actions();

    /* parte un contatore casuale che emetterà una richiesta di taxi */
    alarm(1+rand()%10);
    
    /* INIZIO ESECUZIONE DEI SOURCE */
    /* 
    un source rimane sempre in pausa fino a quando l'alarm (di un tempo casuale) scade e risveglia
    il processo che invierà una richiesta di taxi 
    */
    while (1)
    {
        pause(); /* syscall */
    }

    return(0);
}

void init(int argc, char *argv[]){
    char *s;

    srand(time(NULL)); /* inizializzo il random per i futuri segnali di alarm */
    x = atoi(argv[1]); /* coordinata x del processo corrente, passatagli dal master */
    y = atoi(argv[2]); /* coordinata y del processo corrente, passatagli dal master */

    /* estraggo e assegno le variabili d'ambiente globali che definiranno le dimensioni della matrice di gioco */
    s = getenv("SO_HEIGHT");
    SO_HEIGHT = atoi(s);
    s = getenv("SO_WIDTH");
    SO_WIDTH = atoi(s);
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
            print_map(0); /* da togliere */
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

/* PER TEST DA CANCELLARE AL TERMINE */

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
                    printf("|%c",(char)map[i][k]);
                break;
            }
        }
        /* nuova linea dopo aver finito di stampare le celle della linea i della matrice */
        printf("|\n");
    }
}

int generate_request(){
    int toX, toY, errn, esito; /* coordinate di arrivo della richiesta effettuata da source*/ 

    while (map[toX][toY] == 0)
    {
        toX = rand() % SO_HEIGHT;
        toY = rand() % SO_WIDTH;
    }
    /* inizializzo i parametri nel buffer */
    msg_buffer.mtype = (x * SO_WIDTH) + y;
    if (msg_buffer.mtype <= 0) {
		fprintf(stderr, "\n%s: %d. parametro mtype di una coda di messaggi deve essere > 0. Tentato inserimento di: %d\n", __FILE__, __LINE__, ((x * SO_WIDTH) + y));
		return(-1);
	}
    sprintf(msg_buffer.mtext, "%d", (toX * SO_WIDTH) + toY);

    LOCK_QUEUE;

    if(errn = msgsnd(msq_id, &msg_buffer, MSG_LEN, 0)){
        if((esito = check_snd_msg_status(errn)) == -1)
            exit(esito);
    }

    UNLOCK_QUEUE;
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