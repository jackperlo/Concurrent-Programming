#ifndef COMMUNICATION_H
#define COMMUNICATION_H

/*attendo la sincronizzazione di tutti i figli*/
void wait_for_syncronization(int);

/*blocca i segnali specificati nella maschera*/
void lock_signals(sigset_t);

/*sblocca i segnali specificati nella maschera*/
void unlock_signals(sigset_t);

/*blocca il semaforo di mutua esclusione sulla lettura/scrittura coda di messaggi*/
void lock_queue_semaphore(int);

/*sblocca il semaforo di mutua esclusione sulla lettura/scrittura coda di messaggi*/
void unlock_queue_semaphore(int);

/*blocca il semaforo di mutua esclusione sulla lettura/scrittura nella shared memory*/
void lock_taxi_return_shd_mem_semaphore(int);

/*sblocca il semaforo di mutua esclusione sulla lettura/scrittura nella shared memory*/
void unlock_taxi_return_shd_mem_semaphore(int);

#endif