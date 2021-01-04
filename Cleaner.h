#ifndef CLEANER_H
#define CLEANER_H

/* eseguo la free dello spazio allocato con la malloc durante il riempimento della lista dei parametri */
void free_param_list(param_list); 

/* metodo per pulizia memoria da deallocare */
void cleaner(param_list, int, values_to_source *, int, values_to_taxi *, int, int, int, int **, int **, pid_t **, long int **, int , pid_t *); 

/* esegue la free di tutte le matrici allocate dinamicamente */
/* type: 0 => master, 1 => taxi, 2 => source*/ 
void free_mat(int, int **, int **, pid_t **, long int **, int, pid_t *, taxi_returned_values *); 

#endif