//
// Created by michele on 26/05/17.
//

///@file function.h

#ifndef UNTITLED_FUNCTION_H
#define UNTITLED_FUNCTION_H

// Dichiarazione delle variabili globali
int shmid_a,                                                                                    ///< Valore di ritorno della shmget() per l'area condivisa della matrice A
        shmid_b,                                                                                ///< Valore di ritorno della shmget() per l'area condivisa della matrice B
        shmid_c_molt,                                                                           ///< Valore di ritorno della shmget() per l'area condivisa della moltiplicazione
        shmid_c_sum;                                                                            ///< Valore di ritorno della shmget() per l'area condivisa della somma

int *shared_memory_a,                                                                           ///< Indirizzo virtuale della matrice A
        *shared_memory_b,                                                                       ///< Indirizzo virtuale della matrice B
        *shared_memory_c_molt,                                                                  ///< Indirizzo virtuale della matrice C
        *shared_memory_c_sum;                                                                   ///< Indirizzo virtuale della zona di memoria per la somma della matrice C

int fd_a,                                                                                       ///< File descriptor del file delle matrice B
        fd_b,                                                                                   ///< File descriptor del file delle matrice A
        fd_c;                                                                                   ///< File descriptor del file delle matrice C

pthread_mutex_t mutex ;                                                                         ///< Mutex per le thread

// struttura del messaggio composta da operazione, riga e colonna inviata alla thread
typedef struct {
        int riga;
        int colonna;
        int ordine_matrice;
}message;

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
void *moltiplica(void *value);

// funzione che calcola la somma della riga k della matrice
void *somma(void *value);

// funzione che chiude tutti i processi nel caso in cui venga premuto cun ctrl+c
void close_all();

#endif //UNTITLED_FUNCTION_H
