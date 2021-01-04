#include "Common.h"
#include "Communication.h"

void wait_for_syncronization(int my_sync_semaphore){
    int sem_status;
    sem_status = semctl(my_sync_semaphore, 1, GETVAL); 
    if(sem_status > 0){ /*controllo con l'if per i taxi abortiti che non devono attendere nessun altro taxi per partire con l'esecuzione*/
        s_queue_buff[0].sem_num = 1; 
        s_queue_buff[0].sem_op = -1; /* decrementa il semaforo di 1 */
        s_queue_buff[0].sem_flg = 0;
        while( semop(my_sync_semaphore, s_queue_buff, 1) == -1){
            if(errno != EINTR)
                TEST_ERROR;
        }
        s_queue_buff[0].sem_num = 1;
        s_queue_buff[0].sem_op = 0; /* attende che il semaforo sia uguale a 0 */
        s_queue_buff[0].sem_flg = 0;
        while( semop(my_sync_semaphore, s_queue_buff, 1) == -1){
            if(errno!=EINTR)
                TEST_ERROR;
        }
    }
}

void lock_signals(sigset_t mask){
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void unlock_signals(sigset_t mask){
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}   

void lock_queue_semaphore(int my_queue_semaphore){
    s_queue_buff[0].sem_num = 0;
    s_queue_buff[0].sem_op = -1;
    s_queue_buff[0].sem_flg = 0;
    while( semop(my_queue_semaphore, s_queue_buff, 1) == -1){ /*semop fallita per qualche motivo*/
        if(errno != EINTR) /*EINTR: mentre eseguo la semop mi arriva un segnale*/
            TEST_ERROR;
    }
}

void unlock_queue_semaphore(int my_queue_semaphore){
    s_queue_buff[0].sem_num = 0; 
    s_queue_buff[0].sem_op = 1; 
    s_queue_buff[0].sem_flg = 0;
    while( semop(my_queue_semaphore, s_queue_buff, 1) == -1){
        if(errno != EINTR)
            TEST_ERROR;
    }
}

void lock_taxi_return_shd_mem_semaphore(int my_ret_shd_mem_semaphore){
    s_queue_buff[0].sem_num = 2; 
    s_queue_buff[0].sem_op = -1; 
    s_queue_buff[0].sem_flg = 0;
    while( semop(my_ret_shd_mem_semaphore, s_queue_buff, 1) == -1){
        if(errno != EINTR)
            TEST_ERROR;
    }
}

void unlock_taxi_return_shd_mem_semaphore(int my_ret_shd_mem_semaphore){
    s_queue_buff[0].sem_num = 2;
    s_queue_buff[0].sem_op = 1;
    s_queue_buff[0].sem_flg = 0;
    while( semop(my_ret_shd_mem_semaphore, s_queue_buff, 1) == -1){
        if(errno != EINTR)
            TEST_ERROR;
    }
}