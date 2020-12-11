#include "Common.h"

/*-------------DEFINE di COSTANTI o MACRO--------------*/
/* file eseguibili da cui i figli del master assorbiranno il codice */
#define TAXI "./Taxi"
#define SOURCE "./Source"  
/* path e numero di parametri del file di configurazione per variabili definite a tempo d'esecuzione */
#define SETTING_PATH "./settings"
#define NUM_PARAM 10

/* define richiamata dalla macro TESTERROR definita in common*/
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
void taxi_processes_generator(int number, int pid); /* fork di number processi taxi (utile perche richiamabile anche quando un taxi abortisce e va rigenerato un singolo processo, oltre che la generazione iniziale dei SO_TAXI) */
void free_mat(); /* esegue la free di tutte le matrici allocate dinamicamente */
void execution(); /* esecuzione temporizzata del programma con stampa delle matrici */
void signal_actions(); /* imposta il signal handler */
void print_map_specific(int** m, int isTerminal); /* stampa la mappa passata come parametro */
void signal_handler(int sig); /* gestisce i segnali */
void cpy_mem_values(int what_to_init); /* copia in shd mem i valori presenti in mem locale */
void init_msg_queue(); /* metodo d'appoggio per inizializzare la coda di messaggi */
void cleaner(); /* metodo per pulizia memoria da deallocare */
void init_sem(); /* inizializza semafori */
void kill_all_children(); /* elimina tutti i figli */
int select_a_child_to_do_the_request(); /* seleziona il primo figlio che trova dalla matrice che contiene i pid dei figli e lo destina a alzare la richiesta effettuata via SIGUSR1 */
void signal_sigusr1_actions(); /* gestisce il segnale di richiesta inviato da terminale */
/* metodo d'appoggio che raccoglie l'inizializzazione della shd mem */
/* isTaxi: 1 => inizializza la shd mem per i taxi */
/* isTaxi: 0 => viceversa */
void init_shd_mem(int dim, int isTaxi); 
/* extended_mapping: 0 => convert MAP in struct */
/* extended_mapping: 1 => converte MAP, SO_TIMENSEC_MAP, SO_TOP_CELLS_MAP in una struttura che contiene i rispettivi valori divisi per mappa e per cella*/
void maps_to_struct(int dim, int extended_mapping); 
void get_top_cells(); /* dalla matrice SO_TOP_CELLS_MAP estrae le SO_TOP_CELLS più attraversate */
int check_max_n_taxi(); /* controlla il numero di taxi massimo inseribile nella mappa */

/*-------------VARIABILI GLOBALI-------------*/
int SO_TAXI; /* numero di taxi presenti nella sessione in esecuzione */
int SO_CAP_MIN; /* capacità minima assumibile da SO_CAP: determina il minimo numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
int SO_CAP_MAX; /* capacità massima assumibile da SO_CAP: determina il MASSIMO numero che può assumere il valore che identifica il massimo numero di taxi che possono trovarsi in una cella contemporaneamente */
long int SO_TIMENSEC_MIN; /* valore minimo assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */
long int SO_TIMENSEC_MAX; /* valore MASSIMO assumibile da SO_TIMESEC, che rappresenta il tempo di attraversamento di una cella della matrice */
int **SO_CAP; /* puntatore a matrice di capacità massima per ogni cella */
pid_t **SO_SOURCES_PID; /* puntatore a matrice contenente i PID dei processi SOURCES nella loro coordinata di riferimento */
pid_t *SO_TAXIS_PID; /* puntatore ad array contenente i PID dei processi TAXI */
int seconds = 0; /* contiene i secondi da cui il programma è in esecuzionee */
int SO_DURATION; /* duarata in secondi del programma. Valore definito nel setting */
int shd_id_values_to_source = 0; /* id della shared memory usata dal master coi sources */
int shd_id_values_to_taxi = 0; /* id della shared memory usata dal master coi taxi */
int shd_id_ret_from_taxi = 0; /* id della shareed memory con i valori di ritorno da ogni taxi */
int msg_queue_id; /* id della coda di messaggi per far comunicare sources e taxi */
sigset_t masked; /* maschera per i segnali */ 
struct msqid_ds buf; /* struct che contiene le stats della coda di messaggi. Utile per estrarre tutti i messaggi rimasti non letti */
int SO_TRIP_NOT_COMPLETED = 0; /* numero di viaggi ancora da eseguire o in itinere nel momento della fine dell'esecuzione */
values_to_source *shd_mem_values_to_source; /* stesso typdef della riga sopra, serve per allocare il shd mem uno spazio sufficiente a copiare la map_struct in mem condivisa */
values_to_taxi *shd_mem_values_to_taxi; /* serve per allocare il shd mem uno spazio sufficiente a copiare le maps_struct in mem cond */
taxi_returned_values *shd_mem_taxi_returned_values; /* puntatore a struttura che contiene i 3 parametri da ritornare al master da parte dei */ 
int child_who_does_user_request; /* PID del figlio SO_SOURCE che prenderà in carico la richiesta esplicita da terminale mandata al processo master */
int SO_TOP_CELLS; /* numero di top_cells da stampare */
int SO_TIMEOUT; /* numero di secondi dopo i quali il processo taxi viene abortito */
int SO_TOP_ROAD; /* processo che ha fatto più strada in numero di celle attraversate */
int SO_TOP_LENGTH; /* processo che ha impiegato più tempo(in nsec) in un viaggio */
int SO_TOP_REQ; /* processo che ha completato più request di clienti */
int SO_TRIP_ABORTED; /* numero di viaggi totali abortiti a causa del deadlock di un qualche taxi */

/* funzioni e struttura dati per lettura e gestione parametri su file */
typedef struct node {
	long int value;
	char name[20];
	struct node * next;  
} node;
typedef node* param_list; /* conterrà la lista dei parametri passati tramite file di settings */
param_list listaParametri = NULL; /* lista che contiente i parametri letti dal file di settings */
param_list insert_exec_param_into_list(char name[], int value); /* inserisce i parametri d'esecuzione letti da settings in una lista concatenata */
void print_exec_param_list(); /* stampa la lista dei parametri d'esecuzione */
long int search_4_exec_param(char nomeParam[]); /* ricerca il parametro pssatogli per nome nella lista dei parametri estratti dal file di settings. RITORNA 0 SE NON LO TROVA */
void free_param_list(param_list aus_list); /* eseguo la free dello spazio allocato con la malloc durante il riempimento della lista dei parametri */
int check_n_param_in_exec_list(); /* ritorna il numero di nodi(=parametri) presenti nella lista concatenata. Utile per controllare se ho esattamente gli N_PARAM richiesti */

int main(int argc, char *argv[]){
    char *s;
    /* estraggo e assegno le variabili d'ambiente globali che definiranno le dimensioni della matrice di gioco */
    if((SO_WIDTH <= 0) || (SO_HEIGHT <= 0)){
        fprintf(stderr, "PARAMETRI DI COMPILAZIONE INVALIDI. SO_WIDTH o SO_HEIGHT <= 0.\n");
		exit(EXIT_FAILURE);
    }

    /* per lanciare la richiesta da temrinale: kill -SIGUSR1 <Master-PID> */
    /* stampo il PID del master per essere sfruttato nell'invio del SIGUSR1 */
    dprintf(1, "\nMASTER PROCESS NAME: %d\n", getpid());

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
    
    int n_top_cells = -1; /* definisce la dimensione dell'array che contiene le n_top_cells più attraversate */

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

    SO_TIMENSEC_MAP = (long int **)malloc(SO_HEIGHT*sizeof(long int *));
    if (SO_TIMENSEC_MAP == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_TIMENSEC_MAP[i] = malloc(SO_WIDTH*sizeof(long int));
        if (SO_TIMENSEC_MAP[i] == NULL)
            return;
    }

    SO_SOURCES_PID = (pid_t **)malloc(SO_HEIGHT*sizeof(pid_t *));
    if (SO_SOURCES_PID == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_SOURCES_PID[i] = malloc(SO_WIDTH*sizeof(pid_t));
        if (SO_SOURCES_PID[i] == NULL)
            return;
        for(j=0; j<SO_WIDTH; j++)
            SO_SOURCES_PID[i][j] = 0;
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

    SO_TIMENSEC_MIN = search_4_exec_param("SO_TIMENSEC_MIN");
    SO_TIMENSEC_MAX = search_4_exec_param("SO_TIMENSEC_MAX");

    /* matrice che conterrà il numero di volte in cui ogni cella è stata attraversata. Inizializzata a 0. */
    SO_TOP_CELLS = search_4_exec_param("SO_TOP_CELLS");
    SO_TOP_CELLS_MAP = (int **)malloc(SO_HEIGHT*sizeof(int *));
    if (SO_TOP_CELLS_MAP == NULL)
        return;
    for (i=0; i<SO_HEIGHT; i++){
        SO_TOP_CELLS_MAP[i] = malloc(SO_WIDTH*sizeof(int));
        if (SO_TOP_CELLS_MAP[i] == NULL)
            return;
        for(j=0; j<SO_WIDTH; j++)
            SO_TOP_CELLS_MAP[i][j] = 0;
    }

    SO_TIMEOUT = search_4_exec_param("SO_TIMEOUT");

    /* inizializzo il vettore contenente i pid dei taxi */
    SO_TAXI = search_4_exec_param("SO_TAXI");
    SO_TAXIS_PID = (pid_t *)malloc(SO_TAXI*sizeof(pid_t));

    for(i = 0; i < SO_HEIGHT; i++){
        for(j = 0; j < SO_WIDTH; j++){
            /* genera la matrice delle capacità per ogni cella, genera un valore casuale tra CAP_MIN e CAP_MAX */
            SO_CAP[i][j] = SO_CAP_MIN + rand() % (SO_CAP_MAX - (SO_CAP_MIN - 1));

            /* genera la matrice dei tempi di attesa per ogni cella, genera un valore casuale tra TIMENSEC_MIN e TIMENSEC_MAX */
            SO_TIMENSEC_MAP[i][j] = SO_TIMENSEC_MIN + rand() % (SO_TIMENSEC_MAX - (SO_TIMENSEC_MIN - 1));
        }
    }   

    SO_DURATION = search_4_exec_param("SO_DURATION"); 

    /* genero la mappa */
    map_generator();

    /* inizializzo la shd mem che sfruttero poi dai source per ricavare la costruzione della matrice */
    init_shd_mem( ((SO_WIDTH*SO_HEIGHT) * sizeof(values_to_source)), VALUES_TO_SOURCE);

    /* inizializzo la shd mem che sfruttero poi dai taxi per ricavare la forma della mappa di gioco e la matrice dei tempi di attraversamento per ogni cella*/
    init_shd_mem( ((SO_WIDTH*SO_HEIGHT) * sizeof(values_to_taxi)), VALUES_TO_TAXI);

    /* inizializzo la shd mem in cui i taxi scrivono le loro statistiche; "ritornata" al padre alla loro morte */
    init_shd_mem( ((7+(SO_WIDTH*SO_HEIGHT)) * sizeof(taxi_returned_values)), TAXI_RETURNED_VALUES);

    /* inizializzo la coda di messaggi che verrà utilizzata dai sources e taxi per scambiarsi informazioni riguardo le richieste. Master la sfrutterà per inserire eventuali richieste derivanti da terminale nella coda */
    init_msg_queue();

    /* inizializzo i semafori per message queue e celle mappa */
    init_sem();

    /* genero i processi source */
    source_processes_generator();

    /* genero i processi taxi */
    taxi_processes_generator(SO_TAXI, 0);
}

void map_generator(){
    init_map();
    assign_holes_cells();
    assign_source_cells();
}

void init_map(){
    int i, j;
    /*ctr=0;*/
    for (i = 0; i < SO_HEIGHT; i++){
        for (j = 0; j < SO_WIDTH; j++){
            map[i][j] = 1; /* rendo ogni cella vergine(no sorgente, no inaccessibile) */
            /*dprintf(1, "\nindice: %d => [%d][%d]=%d", ctr, i, j, map[i][j]);
            ctr++;*/
        } 
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
    int esito = 0;
    srand(time(NULL)); /* inizializzo il random number generator */ 
    
    /* estraggo il parametro dalla lista dei parametri; errore se non lo trovo. */
    if((SO_SOURCES = search_4_exec_param("SO_SOURCES")) == 0){
        fprintf(stderr, "Parametro SO_SOURCES non trovato nella lista dei parametri!\n");
		exit(EXIT_FAILURE);
    }

    /* dprintf(1,"\nSO_SOURCES estratti da settings: %d", SO_SOURCES); */
    
    for (i = 0; i < SO_SOURCES; i++){
        do{
            x = rand() % SO_HEIGHT; /* estrae un random tra 0 e (SO_HEIGHT-1) */
            y = rand() % SO_WIDTH; /* estrae un random tra 0 e (SO_WIDTH-1) */
            if(map[x][y] == 1){ /* se la cella non è inaccessibile */
                map[x][y] = 2; /* assegno la cella come SOURCE */
                esito = 1;
                /* dprintf(1, "\nContenuto di map[%d][%d]=%d", x, y, map[x][y]); */
            }
        }while(!esito); /* finché la cella che sto considerando non viene marcata come sorgente */
        esito = 0;
    }

    /* dprintf(1,"\nMAPPA STAMPATA SUBITO DOPO AVER ASSEGNATO I SOURCES...\n"); 
    print_map(0);
    dprintf(1,"\n",SO_SOURCES); */
}

void print_map(int isTerminal){
    /* indici per ciclare */
    int i, k;
    double sec;
    system("clear");
    if(isTerminal){
        sec = (double)shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value/(double)1000000000;
        dprintf(1, "\nNumero totale di viaggi completati: %d", shd_mem_taxi_returned_values[0].completed_trips_counter); /* completed_trips_counter */
        msgctl(msg_queue_id, IPC_STAT, &buf);
        SO_TRIP_NOT_COMPLETED += buf.msg_qnum; /* messaggi di richiesta rimasti non considerati alla morte di tutti i figli: fanno parte dei viaggi inevasi */
        dprintf(1, "\nNumero totale di viaggi inevasi: %d", SO_TRIP_NOT_COMPLETED);
        dprintf(1, "\nNumero totale di viaggi abortiti: %d", SO_TRIP_ABORTED);
        dprintf(1, "\nProcesso che ha fatto più strada in numero di celle: PID:%d ha attraversato %d celle", shd_mem_taxi_returned_values[5].pid, shd_mem_taxi_returned_values[2].total_n_cells_crossed_value); 
        dprintf(1, "\nIl Taxi %d ha fatto il viaggio più lungo nel servire una richiesta:\n\t%dns\n\t~%fs", shd_mem_taxi_returned_values[4].pid, shd_mem_taxi_returned_values[1].max_timensec_complete_trip_value, sec);
        dprintf(1, "\nIl Taxi %d ha raccolto più richieste/clienti; sono ben: %d\n", shd_mem_taxi_returned_values[6].pid, shd_mem_taxi_returned_values[3].max_trip_completed);  
        get_top_cells();
    }

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
                if(SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL) > 0)
                        printf("|%d|" RESET,SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL));
                else
                    printf("|_|" RESET);
                break;
            /* CASO 2: cella sorgente, quadratino striato se stiamo stampando l'ultima mappa, altrimenti stampo una cella generica bianca*/
            case 2:
                if(isTerminal)
                    if(SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL) > 0)
                        printf(BGGREEN KBLACK "|%d|" RESET,SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL));
                    else
                        printf(BGGREEN KBLACK "|_|" RESET);
                else
                    if(SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL) > 0)
                        printf("|%d|",SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL));
                    else
                        printf("|_|");
                break;
            /* DEFAULT: errore o TOP_CELL se stiamo stampando l'ultima mappa, quadratino doppio */
            default:
                if(isTerminal)
                    if(SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL) > 0)
                        printf(BGMAGENTA KBLACK "|%d|" RESET,SO_CAP[i][k] - semctl(sem_cells_id, (i*SO_WIDTH)+k ,GETVAL));
                    else
                        printf(BGMAGENTA KBLACK "|_|" RESET);
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
                        source_args[0] = malloc(7 * sizeof(char));
                        sprintf(source_args[0], "%s", "Source");

                        source_args[1] = malloc(sizeof((char)x));
                        sprintf(source_args[1], "%d", x);

                        source_args[2] = malloc(sizeof((char)y));
                        sprintf(source_args[2], "%d", y);

                        source_args[3] = malloc(sizeof((char)shd_id_values_to_source));
                        sprintf(source_args[3], "%d", shd_id_values_to_source);

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

void taxi_processes_generator(int number, int aborted_pid){
    int i, x, y, valid, k=0; 
    char *taxi_args[9]; /* argomenti passati ai processi taxi */
    srand(time(NULL)*1000*number);

    /* CREO I PROCESSI */
    for (i = 0; i < number; i++){
        valid = 0;
        do{
            x = rand() % SO_HEIGHT;
            y = rand() % SO_WIDTH;
            if(map[x][y]!=0 && semctl(sem_cells_id, (x*SO_WIDTH)+y, GETVAL) > 0){ /* semaforo di indice (x*SO_WIDTH)+y contiene un valore > 0*/
                s_cells_buff[0].sem_num = (x*SO_WIDTH)+y; 
                s_cells_buff[0].sem_op = -1; 
                s_cells_buff[0].sem_flg = IPC_NOWAIT;
                if(semop(sem_cells_id, s_cells_buff, 1) == -1){
                    if(errno != EAGAIN && errno != EINTR){
                        TEST_ERROR;
                    }
                }else{
                    valid = 1;
                }
            }
        }while(!valid);
        
        if(aborted_pid==0) /*non siamo nel caso che rigenera un processo abortito*/
            k=i;
        else{
            for(k=0;k<SO_TAXI;k++){
                if(SO_TAXIS_PID[k] == aborted_pid)
                    break;
            }
        }
        
        switch(SO_TAXIS_PID[k] = fork()){
            case -1:
                /* errore nella fork */
                fprintf(stderr,"\nFORK Error #%03d: %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
                break;
            
            /* caso PROCESSO FIGLIO */ 
            case 0:
                taxi_args[0] = malloc(5 * sizeof(char));
                sprintf(taxi_args[0], "%s", "Taxi");

                taxi_args[1] = malloc(sizeof((char)x));
                sprintf(taxi_args[1], "%d", x);

                taxi_args[2] = malloc(sizeof((char)y));
                sprintf(taxi_args[2], "%d", y);

                taxi_args[3] = malloc(sizeof((char)shd_id_values_to_taxi));
                sprintf(taxi_args[3], "%d", shd_id_values_to_taxi);

                taxi_args[4] = malloc(sizeof((char)msg_queue_id));
                sprintf(taxi_args[4], "%d", msg_queue_id);

                taxi_args[5] = malloc(sizeof((char)sem_sync_id));
                sprintf(taxi_args[5], "%d", sem_sync_id);

                taxi_args[6] = malloc(sizeof((char)SO_TIMEOUT));
                sprintf(taxi_args[6], "%d", SO_TIMEOUT);

                taxi_args[7] = malloc(sizeof((char)shd_id_ret_from_taxi));
                sprintf(taxi_args[7], "%d", shd_id_ret_from_taxi);

                taxi_args[8] = malloc(sizeof((char)sem_cells_id));
                sprintf(taxi_args[8], "%d", sem_cells_id);

                taxi_args[9] = NULL;

                execvp(TAXI, taxi_args);

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
		printf("\nNOME: %s ---- VALORE: %ld", aus_param_list->name, aus_param_list->value);
	}
	printf("\n");
}

long int search_4_exec_param(char nomeParam[]){
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
    long int *currentLongPtr;
    pid_t *currentPid_tPtr;
    /* free map 2d array */
    for (i = 0; i < SO_HEIGHT; i++){
        currentIntPtr = map[i];
        free(currentIntPtr);

        currentIntPtr = SO_CAP[i];
        free(currentIntPtr);

        currentLongPtr = SO_TIMENSEC_MAP[i];
        free(currentLongPtr);

        currentPid_tPtr = SO_SOURCES_PID[i];
        free(currentPid_tPtr);

        currentIntPtr = SO_TOP_CELLS_MAP[i];
        free(currentIntPtr);
    }
}

void execution(){
    int status, pid;

    /* associo gli handlers */
    signal_actions();

    dprintf(1, "\nLegenda della mappa: ");
    dprintf(1, "\n\tCelle INVALIDE:" BGRED "\t  " RESET "\n");
    dprintf(1, "\tCelle SORGENTI:" BGGREEN "\t  " RESET "\n");
    dprintf(1, "\tCelle TOP:" BGMAGENTA "\t  " RESET "\n\n");
    /* faccio un alarm di un secondo per far iniziare il ciclo di stampe ogni secondo */
    dprintf(1, "Attesa Sincronizzazione...\n");
    s_queue_buff[0].sem_num = 1;
    s_queue_buff[0].sem_op = 0; 
    s_queue_buff[0].sem_flg = 0;
    semop(sem_sync_id, s_queue_buff, 1);

    signal_sigusr1_actions(); /* gestisco il segnale SIGUSR1 solo da questo punto in avanti (ovvero quando tutti i processi figli sono pronti all'esecuzione) cosi da poter poi commissionare la richiesta da terminale ad uno di loro */
    
    alarm(1);
    while ((pid = wait(&status)) > 0){
        /* codice di ritorno dei taxi che stavano eseguendo una corsa mentre sono morti */
        if(WEXITSTATUS(status) == TAXI_NOT_COMPLETED_STATUS)
            SO_TRIP_NOT_COMPLETED++;
        else if(WEXITSTATUS(status) == TAXI_ABORTED_STATUS){
            dprintf(1, KRED "\nTAXI ABORTITO!" RESET);
            SO_TRIP_ABORTED++;
            taxi_processes_generator(1, pid);
        }
    }; /* aspetta che siano terminati tutti i suoi figli */

    /* esecuzione terminata */
    printf("\n\n--------------------------------------\nFine: %d secondi di simulazione\n", seconds);
    print_map(1);
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

    /* maschera per indicare i segnali bloccati durante l’esecuzione dell’handler */
    sigaddset(&masked, SIGALRM);
    sigaddset(&masked, SIGINT);

    restart.sa_mask = masked;
    abort.sa_mask = masked;

    sigaction(SIGALRM, &restart, NULL);
    TEST_ERROR;
    sigaction(SIGINT, &abort, NULL);
    TEST_ERROR;
}

void signal_sigusr1_actions(){
    struct sigaction user_signal;
    bzero(&user_signal, sizeof(user_signal));
    user_signal.sa_handler = signal_handler;
    user_signal.sa_flags = SA_RESTART; /* SIGUSR1 di default action ha term */
    sigaddset(&masked, SIGUSR1);
    user_signal.sa_mask = masked;

    sigaction(SIGUSR1, &user_signal, NULL);
    TEST_ERROR;
}

void signal_handler(int sig){
    int ret = -1;

    sigprocmask(SIG_BLOCK, &masked, NULL);
    switch (sig)
    {
        case SIGALRM:
            seconds++;
            printf("\n\n--------------------------------------\nSecondo: %d\n", seconds);
            print_map(0);
            if(seconds < SO_DURATION){
                alarm(1);
            }else{
                kill_all_children();
            }
            break;
        case SIGINT:
            kill_all_children();
            alarm(0);
            break;
        /* richiesta esplicita da terminale, fatta dall'utente. Handler associato solo dopo che tutti i processi sono sincronizzati */
        case SIGUSR1:
            if((child_who_does_user_request = select_a_child_to_do_the_request())==-1)
                dprintf(1, "\nErrore sul SIGUSR1: nessun processo figlio ha preso in carico la richiesta. Riprova");
            else
                dprintf(1, "\nHO SCELTO IL FIGLIO CON PID:%d\n",child_who_does_user_request);
                kill(child_who_does_user_request, SIGUSR1);
                TEST_ERROR;
            break;
        default:
            dprintf(1, "\nSignal %d not handled\n", sig);
            break;
    }
    sigprocmask(SIG_UNBLOCK, &masked, NULL);
}

int select_a_child_to_do_the_request(){
    int x=0, y=0, pid=-1;

    while((pid==-1) && (x<SO_WIDTH) && (y<SO_HEIGHT)){
        if(SO_SOURCES_PID[x][y] != 0) /* matrice che contiene i pid dei figli, il primo che trovo sarà il figlio che alzerà la richiesta aperta via SIGUSR1 */
            pid=SO_SOURCES_PID[x][y];
        x++;
        y++;
    }
    return pid;
}

void kill_all_children(){
    int i,j;

    for(i=0; i<SO_HEIGHT; i++){
        for(j=0; j<SO_WIDTH; j++){
            if(SO_SOURCES_PID[i][j] > 0)
                kill(SO_SOURCES_PID[i][j], SIGQUIT);
        }
    }
    for(i=0; i<SO_TAXI; i++){
        if(SO_TAXIS_PID[i] > 0)
            kill(SO_TAXIS_PID[i], SIGQUIT);
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

void init_shd_mem(int dim, int what_to_init){ 
    if(what_to_init == VALUES_TO_SOURCE){
        /*INIZIALIZZO LA SHARED MEMORY PER I SOURCE */
        shd_id_values_to_source = shmget(IPC_PRIVATE, dim, IPC_CREAT | IPC_EXCL | 0600);
        TEST_ERROR;

        shd_mem_values_to_source = shmat(shd_id_values_to_source, NULL, 0);
        TEST_ERROR;

        cpy_mem_values(what_to_init); /* copia i valori locali in mem condivisa */
        
        /* la shd mem verrà deallocata non appena tutti i processi si staccano */
        shmctl(shd_id_values_to_source, IPC_RMID, NULL);
    }else if(what_to_init == VALUES_TO_TAXI){
        /*INIZIALIZZO LA SHARED MEMORY PER I TAXI */
        shd_id_values_to_taxi = shmget(IPC_PRIVATE, dim, IPC_CREAT | IPC_EXCL | 0600); 
        TEST_ERROR;

        shd_mem_values_to_taxi = shmat(shd_id_values_to_taxi, NULL, 0);
        TEST_ERROR;

        cpy_mem_values(what_to_init);

        /* la shd mem verrà deallocata non appena tutti i processi si staccano */
        shmctl(shd_id_values_to_taxi, IPC_RMID, NULL);
    }else{
        /*INIZIALIZZO LA SHARED MEMORY PER I TAXI */
        shd_id_ret_from_taxi = shmget(IPC_PRIVATE, dim, IPC_CREAT | IPC_EXCL | 0600); 
        TEST_ERROR;

        shd_mem_taxi_returned_values = shmat(shd_id_ret_from_taxi, NULL, 0);
        TEST_ERROR;

        /* pulisce la shared memory: setta dim bytes a 0 */
        bzero(shd_mem_taxi_returned_values, dim);

        /* la shd mem verrà deallocata non appena tutti i processi si staccano */
        shmctl(shd_id_ret_from_taxi, IPC_RMID, NULL);
    }
}

void cpy_mem_values(int what_to_init){
    int i, j;
    long ctr;

    if(what_to_init == VALUES_TO_SOURCE){
        ctr=0;
        for(i = 0; i < SO_HEIGHT; i++){
            for(j = 0; j < SO_WIDTH; j++){
                (shd_mem_values_to_source+ctr)->cell_map_value = map[i][j];
                ctr++;
            }
        } 
    }else{
        ctr=0;
        for(i = 0; i < SO_HEIGHT; i++){
            for(j = 0; j < SO_WIDTH; j++){
                (shd_mem_values_to_taxi+ctr)->cell_map_value = map[i][j];
                (shd_mem_values_to_taxi+ctr)->cell_timensec_map_value = SO_TIMENSEC_MAP[i][j];
               ctr++;
            }
        }
    }
}

void init_msg_queue(){
    msg_queue_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0666);
    TEST_ERROR;
}

void init_sem(){
    int i, minC, maxC;

    /* semaforo message queue */
    sem_sync_id = semget(IPC_PRIVATE, 3, IPC_CREAT|IPC_EXCL| S_IRUSR | S_IWUSR);
    TEST_ERROR;

    sem_arg.val = 1;

    semctl(sem_sync_id, 0, SETVAL, sem_arg);
    TEST_ERROR;

    sem_arg.val = search_4_exec_param("SO_SOURCES") + search_4_exec_param("SO_TAXI");
    semctl(sem_sync_id, 1, SETVAL, sem_arg);
    TEST_ERROR;
    
    sem_arg.val = 1; /* mutex returned value */
    semctl(sem_sync_id, 2, SETVAL, sem_arg);
    TEST_ERROR;
    
    /* inizializza i semafori delle celle */
    sem_cells_id = semget(IPC_PRIVATE, SO_HEIGHT*SO_WIDTH, IPC_CREAT|IPC_EXCL| S_IRUSR | S_IWUSR);
    TEST_ERROR;

    for(i=0;i<SO_HEIGHT*SO_WIDTH;i++){
        sem_arg.val = SO_CAP[i/SO_WIDTH][i%SO_WIDTH];
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
    if(shd_id_values_to_source)
        shmdt(shd_mem_values_to_source);
    if(shd_id_values_to_taxi)
        shmdt(shd_mem_values_to_taxi);

    /* dealloco la coda di messaggi e ottengo i messaggi rimasti non letti nella coda */
    msgctl(msg_queue_id, IPC_RMID, NULL);

    if(sem_sync_id){
        semctl(sem_sync_id, 0, IPC_RMID);
        semctl(sem_sync_id, 1, IPC_RMID);
        semctl(sem_sync_id, 2, IPC_RMID);
    }

    if(sem_cells_id)
        for(i=0;i<SO_HEIGHT*SO_WIDTH;i++){
            semctl(sem_cells_id, i, IPC_RMID);
        } 
}

void get_top_cells(){
    int i, j, aus=0;
    int *top_cells_list;

    top_cells_list = (int *)malloc(SO_TOP_CELLS*sizeof(int));
    if (top_cells_list == NULL)
        return;
    
    for(i=0; i<SO_TOP_CELLS; i++)
        top_cells_list[i] = 0;

    for(j=0; j<SO_TOP_CELLS; j++){
        for(i=7; i<(SO_HEIGHT*SO_WIDTH) + 7; i++){
            if((top_cells_list[j] < shd_mem_taxi_returned_values[i].cell_crossed_map_counter) && (map[(i-7)/SO_WIDTH][(i-7)%SO_WIDTH] != 3)){
                top_cells_list[j] = shd_mem_taxi_returned_values[i].cell_crossed_map_counter;
                aus = i;
            }
        }
        if(aus != 0){
            map[(aus-7)/SO_WIDTH][(aus-7)%SO_WIDTH] = 3;
            aus = 0;
        }
    }

    free(top_cells_list);
}

int check_max_n_taxi(){
    int i, j, counter=0;
    for (i = 0; i < SO_HEIGHT; i++)
    {
        for (j = 0; j < SO_WIDTH; j++){
            if(map[i][j] != 0)
                counter += SO_CAP[i][j];
        }
    }
    return counter;
}