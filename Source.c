#include "Common.h"
#include "Communication.h"
#include "Cleaner.h"

#define EXIT_FAILURE_CUSTOM -1

int x, y;
int timing = 1; /* inizializzazione tempo random dopo cui effettuare la prossima richiesta */
int requests = 0; /* numero di richieste effettuate da questo sorgente */
int msg_queue_id = 0; /* id del semaforo per la coda di messaggi */
sigset_t masked, all; /* maschere per i segnali */ 
values_to_source *shd_map; /* matrice sottoforma di array di stuct che contiene il solo valore della cella, allocata in shd mem e passata dal master */
struct msgbuf msg_buffer; /*contenitore messaggi presenti in coda di messagi*/
int sem_sync_id = -1; /*id semaforo per mutua esclusione su coda di messaggi*/

void init(int argc, char *argv[]); /* funzione di inizializzazione per le variabili globali al processo source e la mappa */
void struct_to_map(); /* converte la struttura che contiene i valori della mappa condivisa passata dalla shd mem in una mappa locale (int **map) */
void init_map(); /* inizializza la mappa locale a source */
void signal_actions(); /* gestione dei segnali */ 
void signal_handler(int sig); /* handler custom sui segnali gestiti */
void generate_request();  /* metodo di supporto che genera e inserisce la richiesta nella coda di messaggi, con gestione relativi semafori di mutua esclusione */
int check_snd_msg_status(int errn); /* controlla l'esito di una message send */
void check_map_to_allow_requests(); /* controlla che nella mappa esista almeno una cella non source e non inaccessibile (che possa quindi essere scelta da destinazione per le richieste) */

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
    wait_for_syncronization(sem_sync_id);

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
    sigdelset(&all, SIGQUIT); /*in forse*/

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
            free_mat(2, map, NULL, NULL, NULL, 0, NULL, NULL);
            exit(requests); 
            break;
        case SIGINT:
            raise(SIGQUIT);
            break;
        /* richiesta esplicita da terminale giunta dal processo master (solo il figlio con uno specifico PID scelto dal padre triggererà questo handler) */
        case SIGUSR1:
            requests++;
            generate_request();
            dprintf(1,"\nSource.c: AGGIUNGO LA RICHIESTA ESPLICITA DA TERMINALE IN CODA! Sono : %d", getpid());
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

void generate_request(){
    int toX, toY, errn, esito; /* coordinate di arrivo della richiesta effettuata da source*/ 
    do
    {
        toX = rand() % SO_HEIGHT;
        toY = rand() % SO_WIDTH;
    }while(map[toX][toY] == 0 || (toX==x && toY==y)); 
    /*
        0->cella inaccessibile
        1->cella vergine
        2->cella source
    */
   
    /* inizializzo i parametri nel buffer */
    msg_buffer.mtype = (x * SO_WIDTH) + y + 1;
    if (msg_buffer.mtype <= 0){
        fprintf(stderr, "\n%s: %d. parametro mtype di una coda di messaggi deve essere > 0. Tentato inserimento di: %d\n", __FILE__, __LINE__, ((x * SO_WIDTH) + y));
        return;
    }
    sprintf(msg_buffer.mtext, "%d", (toX * SO_WIDTH) + toY);

    lock_signals(all);
    lock_queue_semaphore(sem_sync_id);
    
    if( (errn = msgsnd(msg_queue_id, &msg_buffer, MSG_LEN, IPC_NOWAIT) )){ /*IPC_NOWAIT=>se la coda è piena non attendi e vai avanti*/
        if((esito = check_snd_msg_status(errn)) == -1)
            exit(esito);
    }
    
    unlock_queue_semaphore(sem_sync_id);
    unlock_signals(all);
}

int check_snd_msg_status(int errn){
    switch (errno) {
        case EAGAIN:
            /*caso in cui la coda è piena e il tentato msgsnd prosegue senza aspettare che si liberi un posto.*/
            return 0;
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
            return(-1);
	}
}

void check_map_to_allow_requests(){
    int i, j, normal_cells_counter=0;
    
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++){
            if(map[i][j] != 0 && (i!=x && j!=y))
                normal_cells_counter++;
           
        }
    }
    if(normal_cells_counter == 0){
        dprintf(1, "\nSource %d: non ci sono destinazioni possibili.\n", getpid());
        /* attendo che tutti gli altri processi source siano pronti */
        s_queue_buff[0].sem_num = 1; 
        s_queue_buff[0].sem_op = -1; /* decrementa il semaforo di 1 */
        s_queue_buff[0].sem_flg = 0;
        while(semop(sem_sync_id, s_queue_buff, 1)==-1){if(errno!=EINTR)TEST_ERROR;}
        raise(SIGQUIT);
    }    
}