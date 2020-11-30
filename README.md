Master:

    Parametri di Compilazione:
        - SO_WIDTH
        - SO_HEIGHT
        
    Parametri di Esecuzione:
        - SO_HOLES -> definito algoritmicamente dopo la creazione della matrice
                        v | x | v
                        x | x | x
                        x | v | x

        - SO_SOURCES -> al più ( (SO_WIDTH*SO_HEIGHT) - SO_HOLES)
            |
            --> UN PROCESSO PER OGNI SOURCE 
        /*
            Identificano le celle(che non sono INVALIDE) da cui possono
            arrivare una o più richieste da inserire in una CODA DI MESSAGGI -> 
            più versatile perché permette di selezionare i messaggi per "TIPO" 
            (senza doverli per forza leggere in sequenza). Inoltre i messaggi 
            sono DELIMITATI e non stream di byte privi di delimitazioni interne
        */

        - SO_TAXI -> numero di taxi presenti
            |
            --> Un taxi può eseguire un'altra richiesta dopo averne terminata una

        - SO_CAP_MIN -> Minimo assumibile dal numero massimo di taxi che possono essere presenti in una cella contemporaneamente
        - SO_CAP_MAX -> Massimo assumibile dal numero massimo di taxi che possono essere presenti in una cella contemporaneamente
            |
            --> SO_CAP -> rand(SO_CAP_MIN, SO_CAP_MAX)
            //capacità (in numero di taxi) assumibile da una cella di "tipo" SO_SOURCES

        - SO_TIMENSEC_MIN -> Tempo minimo per l'attraversamento di una cella (che non siano INACCESSIBILI)
        - SO_TIMENSEC_MAX -> Tempo massimo per l'attraversamento di una cella (che non siano INACCESSIBILI)
            |
            --> SO_TIMENSEC -> rand(SO_TIMENSEC_MIN, SO_TIMENSEC_MAX)
            //tempo che impiegherà il taxi ad attraversare la prossima cella 



        //sono da passare come parametri o da decidere arbitrariamente nel codice del progetto?
        - SO_TIMEOUT -> se un processo non si muove entro SO_TIMEOUT secondi viene ABORTITO
        
        - SO_DURATION -> durata d'esecuzione (in Secondi) del programma

    Variabili determinate dalla simulazione:

        - SO_TOP_CELLS -> celle più attraversate //quante restituirne (visto che sono "LE CELLE") in ordine decrescente

        - SO_TOP_ROAD -> processo che ha fatto più strada

        - SO_TOP_LENGTH -> processo che ha impiegato più tempo in un viaggio

        - SO_TOP_REQ -> processo che ha raccolto più richieste/clienti

        - SO_TRIP_SUCCESS -> Numero di viaggi completati con successo in totale

        - SO_TRIP_NOT_COMPLETED -> Numero di viaggi non completati o ancora in coda

        - SO_TRIP_ABORTED -> Numero di viaggi abortiti per TIMEOUT


    METODI:
        - int main();

        - void init(); 
        //inizializzazione delle variabili ecc

        - void map_generator(int* mappa[][], SO_HOLES, SO_SOURCES); 
        //genera la matrice con annesse celle HOLES e celle SO_SOURCES

        - void print_map(int* mappa[][], int isTerminal);
        //stampa generale dalla mappa, se isTerminal == 1 stampo la mappa con evidenziate SOURCES e SO_TOP_CELLS
        //chiedere se una cella SOURCE può essere una TOP_CELL 

        - void taxi_generator(SO_TAXI, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT);
        //fa le fork per ogni taxi da creare e gestisce la loro inizializzazione, chiama taxi_exec(SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT);

        - void source_generator(int* mappa[][], SO_SOURCES)
        //genera i processi source associando ad ognuno una cella SOURCE, 
                /*
                e genera il mapping Processo->matriceDellaMappa
                        0 1 2 ... nProcessiSource
                Riga    0|3|1|2|
                Colonna 1|4|3|2|
                dove Riga e Colonna si riferiscono agli indici della matrice di interi che contiene la mappa
                
                */
        DEVE passare durante il fork del nuovo processo source, anche la coppia [riga][col] associata al processo appena forkato source
        
TAXI:

    METODI:
        - void taxi_exec(SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TIMEOUT);
        //gestisce l'esecuzione dei PROCESSI TAXI
SOURCE:

    METODI:
        - void source_exec(int x, int y, int* mappa[][])
