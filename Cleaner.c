#include "Common.h"
#include "Cleaner.h"

void free_param_list(param_list aus_list){
    /* eseguo la free dello spazio allocato per i nodi della lista */
	if (aus_list == NULL) {
		return;
	}
    
	free_param_list(aus_list->next);
    free(aus_list);
}

void cleaner(param_list listaParametri, int shd_id_values_to_source, values_to_source *shd_mem_values_to_source, int shd_id_values_to_taxi, values_to_taxi *shd_mem_values_to_taxi, int msg_queue_id, int sem_sync_id, int sem_cells_id, int **map, int **SO_CAP, pid_t **SO_SOURCES_PID, long int **SO_TIMENSEC_MAP, int free_so_taxis_pid, pid_t *SO_TAXIS_PID){
    int i;
    /* deallocazione malloc matrici */
    free_param_list(listaParametri);
    free_mat(0, map, SO_CAP, SO_SOURCES_PID, SO_TIMENSEC_MAP, free_so_taxis_pid, SO_TAXIS_PID, NULL);

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

void free_mat(int type, int **map, int **SO_CAP, pid_t **SO_SOURCES_PID, long int **SO_TIMENSEC_MAP, int free_so_taxis_pid, pid_t *SO_TAXIS_PID, taxi_returned_values *aus_shd_mem_taxi_returned_values){
    int i;
    int *currentIntPtr;
    long int *currentLongPtr;
    pid_t *currentPid_tPtr;
    
    for (i = 0; i < SO_HEIGHT; i++){
        currentIntPtr = map[i];
        free(currentIntPtr);
        if(type == 0){
            currentIntPtr = SO_CAP[i];
            free(currentIntPtr);
            currentPid_tPtr = SO_SOURCES_PID[i];
            free(currentPid_tPtr);
        }
        if(type == 1){
            currentLongPtr = SO_TIMENSEC_MAP[i];
            free(currentLongPtr);
        }
    }
    if(type == 0){
        if(free_so_taxis_pid)
            free(SO_TAXIS_PID);
        free(map);
        free(SO_CAP);
        free(SO_TIMENSEC_MAP);
        free(SO_SOURCES_PID);
    }
    if(type == 1)
        free(aus_shd_mem_taxi_returned_values);
}
