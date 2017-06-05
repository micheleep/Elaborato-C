//
// Created by michele on 15/05/17.
//

///@file function.h

#ifndef UNTITLED_FUNCTION_H
#define UNTITLED_FUNCTION_H

// Dichiarazione delle variabili globali
int shmid_a,                                                                                    ///< Valore di ritorno della shmget() per l'area condivisa della matrice A
        shmid_b,                                                                                ///< Valore di ritorno della shmget() per l'area condivisa della matrice B
        shmid_c_molt,                                                                           ///< Valore di ritorno della shmget() per l'area condivisa della moltiplicazione
        shmid_c_sum;                                                                            ///< Valore di ritorno della shmget() per l'area condivisa della somma
int id_mess;                                                                                    ///< Valore di ritorno della msgget()
int sem_id;                                                                                     ///< Valore di ritorno della semget()
int fd_a,                                                                                       ///< File descriptor del file delle matrice B
        fd_b,                                                                                   ///< File descriptor del file delle matrice A
        fd_c;                                                                                   ///< File descriptor del file delle matrice C

// struttura del messaggio composta da operazione, riga e colonna inviato tramite pipe
typedef struct {
        char operazione;
        int riga;
        int colonna;
}message;

// struttura del messaggio che il figlio deve inviare al padre tramite coda di messaggi
typedef struct {
        long mtype;                                 // per capire con quale figlio stiamo parlando
        int numero_processo;                        // identificativo del processo che viene mandato in esecuzione

}queue_message;

// union per la gestione delle operazioni sui semafori
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
} stato_sem;

// funzione per la stampa a video
void print(char string[]);

// funzione per stampa su stderror
void print_error(char string[]);

// funzione che stampa un intero a video
void print_integer(int value);

// funzione che controlla se la matrice è quadrata
int controllo_matrice(int file_descriptor);

// conta i caratteri presenti all'interno della matrice
int conta_valori_matrice(int file_descriptor);

// funzione che ritorna ID dove viene creata la zona di memoria
int get_memoria_condivisa_padre(key_t key, int ordine);

// funzione che scrive in memoria condivisa la matrice
int *scrivi_matrice(int shmid, int file_descriptor, int numero_valori, int ordine_matrice);

// funzione che moltiplica la riga per la colonna
void moltiplica(int *shared_memory_a, int *shared_memory_b, int *shared_memory_c_molt, int riga_i, int riga_j, int ordine_matrice);

// funzione che calcola la somma della riga k della matrice
void somma(int *shared_memory_c_molt, int *shared_memory_c_sum, int riga_k, int ordine_matrice);

// funzione che scrive il messaggio sulla struttura
message riempi_struct(char operazione, int riga, int colonna);

// funzione che fa la wait per quanto rigurada i semafori --> aumenta di un valore il semaforo
void sem_dec(int sem_id, int sem_numero);

// funzione che fa la signal per quanti riguarda i semafori --> decrementa di un valore il semaforo
void sem_inc(int sem_id, int sem_numero);

// funzione che chiude tutti i processi nel caso in cui venga premuto cun ctrl+c
void close_all();

#endif //UNTITLED_FUNCTION_H
