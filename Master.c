#include "Common.h"

/*-------------DEFINE di COSTANTI--------------*/
/* file eseguibili da cui i figli del master assorbiranno il codice */
#define TAXI "./Taxi"
#define SOURCE "./Source"  
/* path e numero di parametri del file di configurazione per variabili definite a tempo d'esecuzione */
#define SETTING_PATH "./settings"
#define NUM_PARAM 9

#define CLEAN cleaner();

/*------------DICHIARAZIONE METODI------------*/
void init(); /* inizializzazione delle variabili */
void map_generator(); /* genera la matrice con annesse celle HOLES e celle SO_SOURCES */
void init_map(); /* inizializza la matrice vergine (tutte le celle a 1)*/
void assign_holes_cells(); /* metodo di supporto a map_generator(), assegna le celle invalide */
void assign_source_cells(); /* metodo di supporto a map_generator(), assegna le celle sorgenti */
int check_cell_2be_inaccessible(int x, int y); /* metodo di supporto a map_generator(). controlla se le 8 celle adiacenti a quella considerata sono tutte non inaccessibili */ 
void print_map(int isTerminal); /* stampa una vista della mappa durante l'esecuzione, e con isTerminal evidenzia le SO_TOP_CELLS celle con più frequenza di passaggio */
void source_processes_generator(); /* fork dei processi sorgenti */
void free_mat(); /* esegue la free di tutte le matrici allocate dinamicamente */
void execution(); /* esecuzione temporizzata del programma con stampa delle matrici */
void signal_actions(); /* imposta il signal handler */
void kill_sources(); /* elimina tutti i figli sources */
void print_map_specific(int** m, int isTerminal); /* stampa la mappa passata come parametro */
void signal_handler(int sig); /* gestisce i segnali */
void map_to_struct(int dim); /* convert map into struct */ 
void init_shd_mem(int dim); /* metodo d'appoggio che raccoglie l'inizializzazione della shd mem */
void init_msg_queue(); /* metodo d'appoggio per inizializzare la coda di messaggi */
void cleaner(); /* metodo per pulizia memoria da deallocare */
void init_sem(); /* inizializza semafori */

/*-------------COSTANTI GLOBALI-------------*/
int SO_TAXI; /* numero di taxi presenti nella sessione in esecuzione */
int SO_CAP_MIN; /* capacità minima assumibile da SO_CAP: determina il minimo numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
int SO_CAP_MAX; /* capacità massima assumibile da SO_CAP: determina il MASSIMO numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
int SO_TIMENSEC_MIN; /* valore minimo assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */
int SO_TIMENSEC_MAX; /* valore MASSIMO assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */

/*-------------VARIABILI GLOBALI-------------*/
int **SO_CAP; /* puntatore a matrice di capacità massima per ogni cella */
int **SO_TIMENSEC; /* puntatore a matrice dei tempi di attesa per ogni cella */
pid_t **SO_SOURCES_PID; /* puntatore a matrice contenente i PID dei processi SOURCES nella loro coordinata di riferimento */
int seconds = 0; /* contiene i secondi da cui il programma è in esecuzionee */
int SO_DURATION; /* duarata in secondi del programma. Valore definito nel setting */
int shd_id = 0; /* id della shared memory */
int msg_queue_id; /* id della coda di messaggi per far comunicare sources e taxi */
sigset_t masked; /* maschera per i segnali */ 
struct msqid_ds buf; /* struct che contiene le stats della coda di messaggi. Utile per estrarre tutti i messaggi rimasti non letti */
int SO_TRIP_NOT_COMPLETED = 0; /* numero di viaggi ancora da eseguire o in itinere nel momento della fine dell'esecuzione */
mapping *map_struct;
mapping *shd_map;

/* funzioni e struttura dati per lettura e gestione parametri su file */
typedef struct node {
	int value;
	char name[20];
	struct node * next;  
} node;

typedef node* param_list; /* conterrà la lista dei parametri passati tramite file di settings */
param_list listaParametri = NULL; /* lista che contiente i parametri letti dal file di settings */
param_list insert_exec_param_into_list(char name[], int value); /* inserisce i parametri d'esecuzione letti da settings in una lista concatenata */
void print_exec_param_list(); /* stampa la lista dei parametri d'esecuzione */
int search_4_exec_param(char nomeParam[]); /* ricerca il parametro pssatogli per nome nella lista dei parametri estratti dal file di settings. RITORNA 0 SE NON LO TROVA */
void free_param_list(param_list aus_list); /* eseguo la free dello spazio allocato con la malloc durante il riempimento della lista dei parametri */
int check_n_param_in_exec_list(); /* ritorna il numero di nodi(=parametri) presenti nella lista concatenata. Utile per controllare se ho esattamente gli N_PARAM richiesti */

/*
-------------elenco dei parametri d'esecuzione e loro descrizione------------------
int SO_HOLES;  indica il numero di celle inaccessibili(da cui holes) all'interno della matrice
int SO_SOURCES;  indica il numero di celle sorgenti di possibili richieste. Come fossero le stazioni di servizio per i taxi 
int SO_TIMEOUT;  numero di secondi dopo i quali il processo viene abortito 
int SO_DURATION;  durata dell'esecuzione in secondi 
int SO_TOP_CELLS;  numero di celle più attraversate (es. 4 -> riferisco le prime 4 più attraversate) 
int SO_TOP_ROAD;  processo che ha fatto più strada 
int SO_TOP_LENGTH;  processo che ha impiegato più tempo in un viaggio 
int SO_TOP_REQ;  processo che ha completato più request di clienti 
int SO_TRIP_SUCCESS;  numero di viaggi eseguiti con successo, da stampare a fine dell'esecuzione 
int SO_TRIP_ABORTED;  numero di viaggi abortiti a causa del deadlock 
*/

int main(int argc, char *argv[]){
    char *s;
    /* estraggo e assegno le variabili d'ambiente globali che definiranno le dimensioni della matrice di gioco */
    if((SO_WIDTH <= 0) || (SO_HEIGHT <= 0)){
        fprintf(stderr, "PARAMETRI DI COMPILAZIONE INVALIDI. SO_WIDTH o SO_HEIGHT <= 0.\n");
		exit(EXIT_FAILURE);
    }

    /* CONFIGURAZIONE DELL'ESECUZIONE */
    init();
    
    /* START UFFICIALE DELL'ESECUZIONE */
    execution();
    
    /* CONCLUSIONE DELL'ESECUZIONE */
    cleaner();

    return 0;
}

void init(){
    int i, j;
    FILE *settings;
    /* conterranno nome e valore di ogni parametro definito in settings */
    char name[100];
    int value;
    int check_n_param_return_value = -1; /*conterrà il numero di nodi(=numero di parametri) presenti nella lista che legge i parametri da file di settings */
    int dim = ( (SO_WIDTH*SO_HEIGHT) * sizeof(mapping) * (sizeof(int)) ); /* dimensione allocata per la shd mem */ 

    /*-----------INIZIALIZZO LE MATRICI GLOBALI--------------*/

    map = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (map == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        map[i] = malloc(SO_WIDTH*sizeof(int));
        if (map[i] == NULL)
            return;
    }

    SO_CAP = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (SO_CAP == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_CAP[i] = malloc(SO_WIDTH*sizeof(int));
        if (SO_CAP[i] == NULL)
            return;
    }

    SO_TIMENSEC = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (SO_TIMENSEC == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_TIMENSEC[i] = malloc(SO_WIDTH*sizeof(int));
        if (SO_TIMENSEC[i] == NULL)
            return;
    }

    SO_SOURCES_PID = (pid_t **)malloc(SO_HEIGHT*sizeof(pid_t *));
    if (SO_SOURCES_PID == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_SOURCES_PID[i] = malloc(SO_WIDTH*sizeof(pid_t));
        if (SO_SOURCES_PID[i] == NULL)
            return;
    }


    /*-----------LEGGO I PARAMETRI DI ESECUZIONE-------------*/
    /* inizializzo il random */
    srand(time(NULL));
    /* apro il file settings */
    if((settings = fopen(SETTING_PATH , "r")) == NULL){
        fprintf(stderr, "Errore durante l'apertura file che contiene i parametri\n");
		exit(EXIT_FAILURE);
    }
    for(i=0; i<NUM_PARAM; i++){
        /* leggo ogni riga del file finché non raggiunge la fine EOF */
        while(fscanf(settings, "%s = %d\n", name, &value) != EOF)
            listaParametri = insert_exec_param_into_list(name, value);
    }

    /* check: ho tutti e soli i NUM_PARAM parametri d'esecuzione richiesti? */
    if((check_n_param_return_value = check_n_param_in_exec_list()) != NUM_PARAM){
        fprintf(stderr, "Master.c ERRORE nel numero di parametri presenti nel file di Settings.\nRichiesti:%d\nOttenuti:%d\n", NUM_PARAM, check_n_param_return_value);
		exit(EXIT_FAILURE);
    }

    /* chiudo il file settings */
    fclose(settings);

#ifdef DEBUG
    print_exec_param_list();
#endif

    SO_CAP_MIN = search_4_exec_param("SO_CAP_MIN");
    SO_CAP_MAX = search_4_exec_param("SO_CAP_MAX");

    for(i = 0; i < SO_HEIGHT; i++){
        for(j = 0; j < SO_WIDTH; j++){
            /* genera la matrice delle capacità per ogni cella, genera un valore casuale tra CAP_MIN e CAP_MAX */
            SO_CAP[i][j] = SO_CAP_MIN + rand() % (SO_CAP_MAX - (SO_CAP_MIN - 1));

            /* genera la matrice dei tempi di attesa per ogni cella, genera un valore casuale tra TIMENSEC_MIN e TIMENSEC_MAX */
            SO_TIMENSEC[i][j] = SO_TIMENSEC_MIN + rand() % (SO_TIMENSEC_MAX - (SO_TIMENSEC_MIN - 1));
        }
    }   
    SO_DURATION = search_4_exec_param("SO_DURATION"); 

    /* genero la mappa */
    map_generator();

    /* converte la mappa in una struttura che contiene i valori della matrice */
    map_to_struct(dim);

    /* inizializzo la shd mem che sfruttero poi dai figli per ricavare la costruzione della matrice */
    init_shd_mem(dim);

    /* inizializzo la coda di messaggi che verrà utilizzata dai sources e taxi per scambiarsi informazioni riguardo le richieste. Master la sfrutterà per inserire eventuali richieste derivanti da terminale nella coda */
    init_msg_queue();

    /* inizializzo i semafori per message queue e celle mappa */
    init_sem();

    /* genero i processi source */
    source_processes_generator();
}

void map_generator(){
    init_map();
    assign_holes_cells();
    assign_source_cells();
}

void init_map(){
    int i, j;
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++)
            map[i][j] = 1; /* rendo ogni cella vergine(no sorgente, no inaccessibile) */
    }
}

void assign_holes_cells(){
    int i, x, y, esito=0; /* valore restituito dalla check_cell_2be_inaccessible(): 0 -> cella non adatta ad essere inaccessibile per vincoli di progetto. 1 -> cella adatta ad essere inaccessibile */
    int SO_HOLES;
    srand(time(NULL)); /* inizializzo il random number generator */ 

    /* estraggo il parametro dalla lista dei parametri; errore se non lo trovo */
    
    if((SO_HOLES = search_4_exec_param("SO_HOLES")) == 0){
        fprintf(stderr, "Parametro SO_HOLES non trovato nella lista dei parametri!\n");
		exit(EXIT_FAILURE);
    }

    for (i = 0; i < SO_HOLES; i++){
        do{
        
            x = rand() % SO_HEIGHT; /* estrae un random tra 0 e (SO_HEIGHT-1) */  
            y = rand() % SO_WIDTH; /* estrae un random tra 0 e (SO_WIDTH-1) */    
            if(map[x][y] != 0) /* se la cella non è già segnata come inaccessibile */
                esito = check_cell_2be_inaccessible(x, y);
        }while(esito == 0); /* finché non trovo una cella adatta ad essere definita inaccessibile */
        map[x][y] = 0;  /* rendo effettivamente la cella inaccessibile */
        esito = 0; 
    }
}

void assign_source_cells(){
    int i, x, y;
    int SO_SOURCES;
    srand(time(NULL)); /* inizializzo il random number generator */ 
    
    /* estraggo il parametro dalla lista dei parametri; errore se non lo trovo. */
    if((SO_SOURCES = search_4_exec_param("SO_SOURCES")) == 0){
        fprintf(stderr, "Parametro SO_SOURCES non trovato nella lista dei parametri!\n");
		exit(EXIT_FAILURE);
    }

    /*dprintf(1,"%d",SO_SOURCES);*/
    
    for (i = 0; i < SO_SOURCES; i++){
        do{
            x = rand() % SO_HEIGHT; /* estrae un random tra 0 e (SO_HEIGHT-1) */
            y = rand() % SO_WIDTH; /* estrae un random tra 0 e (SO_WIDTH-1) */
            if(map[x][y] != 0) /* se la cella non è inaccessibile */
                map[x][y] = 2; /* assegno la cella come SOURCE */
        }while(map[x][y] != 2); /* finché la cella che sto considerando non viene marcata come sorgente */
    }
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

void source_processes_generator(){
    int x,y; 
    char *source_args[8]; /* argomenti passati ai processi source */

    /* CREO I PROCESSI */
    for (x = 0; x < SO_HEIGHT; x++){
        for (y = 0; y < SO_WIDTH; y++){
            if(map[x][y] == 2){
                switch(SO_SOURCES_PID[x][y] = fork()){
                    case -1:
                        /* errore nella fork */
                        fprintf(stderr,"\nFORK Error #%03d: %s\n", errno, strerror(errno));
                        exit(EXIT_FAILURE);
                        break;
                    
                    /* caso PROCESSO FIGLIO */ 
                    case 0:
                        source_args[0] = malloc(10 * sizeof(char));
                        sprintf(source_args[0], "%s", "Source");

                        source_args[1] = malloc(sizeof((char)x));
                        sprintf(source_args[1], "%d", x);

                        source_args[2] = malloc(sizeof((char)y));
                        sprintf(source_args[2], "%d", y);

                        source_args[3] = malloc(sizeof((char)shd_id));
                        sprintf(source_args[3], "%d", shd_id);

                        source_args[4] = malloc(sizeof((char)msg_queue_id));
                        sprintf(source_args[4], "%d", msg_queue_id);

                        source_args[5] = malloc(sizeof((char)sem_sync_id));
                        sprintf(source_args[5], "%d", sem_sync_id);

                        source_args[6] = NULL;

                        execvp(SOURCE, source_args);

                        /* ERRORE ESECUZIONE DELLA EXECVP */
                        fprintf(stderr, "\n%s: %d. EXECVP Error #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
	                    exit(EXIT_FAILURE);
                        break;
                    
                    /* caso PROCESSO PADRE */
                    default:
                        break;
                }
                
            }
        }
    }
}

param_list insert_exec_param_into_list(char name[], int value){
    param_list new_elem;
    int esito = 1; /* controllo che non si stiano inserendo parametri doppi */
    esito = search_4_exec_param(name);

    if(esito == 0){ /* se non ancora presente nella lista */
        /* genero un nuovo nodo(name,value) della lista e lo inserisco all'interno della stessa */
        new_elem = malloc(sizeof(*new_elem));
        new_elem->value = value;
        strcpy(new_elem->name, name);
        new_elem->next = listaParametri;
        return new_elem;
    }else{
        fprintf(stderr, "\n%s: %d. PARAMETRO DUPLICATO nel file di SETTING.\nNome del parametro doppio: %s\n", __FILE__, __LINE__, name);
        exit(EXIT_FAILURE);
    }
}

void print_exec_param_list(){
    /* stampa dei nodi contenuti nella lista */
    param_list aus_param_list = listaParametri;
	if (aus_param_list == NULL) {
		printf("Empty EXECUTION PARAM LIST\n");
		return;
	}
	for(; aus_param_list!=NULL; aus_param_list = aus_param_list->next) {
		printf("\nNOME: %s ---- VALORE: %d", aus_param_list->name, aus_param_list->value);
	}
	printf("\n");
}

int search_4_exec_param(char nomeParam[]){
    param_list aus_param_list = listaParametri;
	for(; aus_param_list!=NULL; aus_param_list = aus_param_list->next) {
        if (strcmp(aus_param_list->name,nomeParam) == 0){
            return aus_param_list->value;
        }
			
    }
	return 0;
}

int check_n_param_in_exec_list(){
    int n_elem = 0;
    param_list aus_param_list = listaParametri;
	for(; aus_param_list!=NULL; aus_param_list = aus_param_list->next) {
        n_elem++;	
    }
	return n_elem;
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

void free_param_list(param_list aus_list){
    /* eseguo la free dello spazio allocato per i nodi della lista */
	if (aus_list == NULL) {
		return;
	}
    
	free_param_list(aus_list->next);
    free(aus_list);
}

void free_mat(){
    int i;
    int *currentIntPtr;
    pid_t *currentPid_tPtr;
    /* free map 2d array */
    for (i = 0; i < SO_HEIGHT; i++){
        currentIntPtr = map[i];
        free(currentIntPtr);

        currentIntPtr = SO_CAP[i];
        free(currentIntPtr);

        currentIntPtr = SO_TIMENSEC[i];
        free(currentIntPtr);

        currentIntPtr = SO_SOURCES_PID[i];
        free(currentIntPtr);
    }
}

void execution(){
    int status, pid;
    /* associa l'handler dell'alarm */
    signal_actions();

    /* faccio un alarm di un secondo per far iniziare il ciclo di stampe ogni secondo */
    dprintf(1, "Attesa Sincronizzazione...\n");
    s_queue_buff[0].sem_num = 1;
    s_queue_buff[0].sem_op = 0; 
    s_queue_buff[0].sem_flg = 0;
    semop(sem_sync_id, s_queue_buff, 1);
    alarm(1);
    while ((pid = wait(&status)) > 0); /* aspetta che siano terminati tutti i suoi figli */

    /* esecuzione terminata */
    printf("\n\n--------------------------------------\nFine: %d secondi di simulazione\n", seconds);
    print_map(1);
}

void signal_actions(){
    struct sigaction restart, abort; 

    /* pulisce le struct */
    bzero(&restart, sizeof(restart));
    bzero(&abort, sizeof(abort));

    restart.sa_handler = signal_handler;
    abort.sa_handler = signal_handler;

    restart.sa_flags = SA_RESTART;

    sigaddset(&masked, SIGALRM);
    sigaddset(&masked, SIGCHLD);
    sigaddset(&masked, SIGINT);

    restart.sa_mask = masked;
    abort.sa_mask = masked;

    sigaction(SIGALRM, &restart, NULL);
    TEST_ERROR;
    sigaction(SIGCHLD, &restart, NULL);
    TEST_ERROR;
    sigaction(SIGINT, &abort, NULL);
    TEST_ERROR;
}

void signal_handler(int sig){
    sigprocmask(SIG_BLOCK, &masked, NULL);
    switch (sig)
    {
        /* Generazione Richiesta */
        case SIGALRM:
            seconds++;
            printf("\n\n--------------------------------------\nSecondo: %d\n", seconds);
            print_map(0);
            if(seconds < SO_DURATION){
                alarm(1);
            }else{
                kill_sources();
        /*      kill_taxi();    */
            }
            break;
        case SIGCHLD:

            break;
        case SIGINT:
            kill_sources();
            alarm(0);
            break;
        default:
            dprintf(1, "\nSignal %d not handled\n", sig);
            break;
    }
    sigprocmask(SIG_UNBLOCK, &masked, NULL);
}

void kill_sources(){
    int i,j;

    for(i=0; i<SO_HEIGHT; i++){
        for(j=0; j<SO_WIDTH; j++){
            if(SO_SOURCES_PID[i][j] > 0)
                kill(SO_SOURCES_PID[i][j], SIGQUIT);
        }
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

void map_to_struct(int dim){
    int i, j, ctr = 0;

    map_struct = (mapping*)malloc(dim);
    for(i = 0; i < SO_HEIGHT; i++){
        for(j = 0; j < SO_WIDTH; j++){
            map_struct[ctr].value = map[i][j];
            ctr++;
        }
    }
}

void init_shd_mem(int dim){
    /*INIZIALIZZO LA SHARED MEMORY PER MAP */
    shd_id = shmget(IPC_PRIVATE, sizeof(dim), IPC_CREAT | IPC_EXCL | 0600);
    TEST_ERROR;
    
    shd_map = (mapping *)shmat(shd_id, NULL, 0);
    TEST_ERROR;

    /* la shd mem verrà deallocata non appena tutti i processi si staccano */
    shmctl(shd_id, IPC_RMID, NULL);
    memcpy(shd_map, map_struct, dim);
}

void init_msg_queue(){
    msg_queue_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666); /* AGGIUNGERE IPC_EXCL */
    TEST_ERROR;
}

void init_sem(){
    int i, minC, maxC;

    /* semaforo message queue */
    sem_sync_id = semget(IPC_PRIVATE, 2, IPC_CREAT|IPC_EXCL| S_IRUSR | S_IWUSR);
    TEST_ERROR;

    sem_arg.val = 1;

    semctl(sem_sync_id, 0, SETVAL, sem_arg);
    TEST_ERROR;
    sem_arg.val = search_4_exec_param("SO_SOURCES");
    semctl(sem_sync_id, 1, SETVAL, sem_arg);
    TEST_ERROR;
    
    /* inizializza i semafori delle celle */
    sem_cells_id = semget(IPC_PRIVATE, SO_HEIGHT*SO_WIDTH, IPC_CREAT|IPC_EXCL| S_IRUSR | S_IWUSR);
    TEST_ERROR;

    for(i=0;i<SO_HEIGHT*SO_WIDTH;i++){
        sem_arg.val = SO_CAP[i%SO_WIDTH][i/SO_WIDTH];
        semctl(sem_cells_id, i, SETVAL, sem_arg);
        TEST_ERROR;
    } 
}

void cleaner(){
    int i;
    /* deallocazione malloc matrici */
    free_param_list(listaParametri);
    free_mat();

    /* deallocazione memoria condivisa */
    if(shd_id)
        shmdt(shd_map);

    /* dealloco la coda di messaggi e ottengo i messaggi rimasti non letti nella coda */
    msgctl(msg_queue_id, IPC_STAT, &buf);
    SO_TRIP_NOT_COMPLETED += buf.msg_qnum;
    dprintf(1,"Numero trip non completati: %d", SO_TRIP_NOT_COMPLETED);
    msgctl(msg_queue_id, IPC_RMID, NULL);
    

    if(sem_sync_id){
        semctl(sem_sync_id, 0, IPC_RMID);
        semctl(sem_sync_id, 1, IPC_RMID);
    }

    if(sem_cells_id)
        for(i=0;i<SO_HEIGHT*SO_WIDTH;i++){
            semctl(sem_cells_id, i, IPC_RMID);
        } 
}