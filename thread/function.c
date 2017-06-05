//
// Created by michele on 26/05/17.
//

///@file function.c

#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include "function.h"

/**
 * Per fare la stampa a video utilizzando la sys_call write su stdout
 *
 * @param puntatore ad una stringa qualsiasi
 */
void print(char string[]) {
    if((write(1, string, strlen(string))) == -1) {
        close_all();
        exit(1);
    }
}

/**
 * Per fare la stampa a video utilizzando la sys_call write su stderror
 *
 * @param puntatore ad una stringa qualsiasi
 */
void print_error(char string[]){
    write(2, string, strlen(string));
    close_all();
    exit(-1);
}

/**
 * Funzione che vuole in input un intero e che lo converte con sprintf in una stringa
 * Utilizza la funzione print che usa la sys call write()
 *
 * @param value
 */
void print_integer(int value){
    char tmp[5];                                                        // variabile di supporto per la stampa a video
    sprintf(tmp, "%d", value);                                          // scrivo in tmp il valore del risultato, come se fosse una stringa
    print(tmp);                                                         // con la write scrivo il valore di tmp
}

/**
 * Controlla che la matrice sia quadrata
 *
 * @param file_descriptor
 * @return un valore intero che corrisponde all'ordine della matrice calcolato
 */
int controllo_matrice(int file_descriptor){

    char c;                                                                 // buffer di lettura/scrittura
    int valori_riga=1;                                                      // numero delle righe della matrice
    int valori_colonna=1;                                                   // numero delle colonne della matrice
    int valori_colonna_old=0;                                               // variabile che serve per il controllo delle righe della matrice
    bool matrice_corretta=true;                                             // per vedere se la matrice è effettivamente QUADRATA e quindi se è possibile operare su di essa
    bool flag_prima_volta=true;                                             // flag per l'inizializzazione di old_value per la prima volta che viene eseguito il ciclo

    while ((read(file_descriptor, &c, 1) == 1) && matrice_corretta) {
        if (c == ';')                                                       // conto il numero di valori che ha la matrice su ogni riga
            valori_colonna++;                                               // aumento il valore delle colonne

        if (c == 10) {                                                      // quando ho terminato una riga devo controllare che sia uguale a tutte le altre
            if (flag_prima_volta) {                                         // solo la prima volta, per inizializzare il valore old_value
                flag_prima_volta = false;
                valori_colonna_old = valori_colonna;
            }

            if (valori_colonna != valori_colonna_old)                       // se il valore nuovo è diverso da quello vecchio posso dire che non è quadrata
                matrice_corretta = false;
            else {
                valori_colonna_old = valori_colonna;                        // se è tutto andato ok aggiorno i valori e 'riparto' con il ciclo
                valori_colonna = 1;
                valori_riga++;
            }
        }

    }

    if (valori_riga-1 == valori_colonna_old)
        return valori_colonna_old;
    else
        return 0;
}

/**
 * Funzione che crea la memoria condivisa e ne ritorna l'id
 *
 * @param key
 * @param ordine
 * @return il valore dove viene creata la memoria condivisa
 */
int get_memoria_condivisa_padre(key_t key, int ordine){

    int shmid = 0;

    if ((shmid = shmget(key, ordine*ordine*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL)) == -1){       // creo l'area di memoria condivisa, con IPC_CREAT
        print_error("Errore durante la creazione del segmento di memoria in calcola.c!\n");          // controllo il valore restituito, se -1 errore
        close_all();
        return 0;
    }

    return shmid;                                                                                   // ritorno il valore corretto
}

/**
 * Conta i caratteri che sono all'interno della matrice
 *
 * @param file_descriptor
 * @return il numero di caratteri letti
 */
int conta_valori_matrice(int file_descriptor){

    char *c;
    int counter = 0;

    lseek(file_descriptor, 0, 0);                               // mi posiziono all'inizio del file

    while (read(file_descriptor, &c, 1))                        // conto tutti i valori (caratteri) che sono presenti all'interno del file
        counter++;

    return counter;                                             // ritorno il numero corretto di caratteri
}

/**
 * Inserisce in memoria condivisa la matrice che viene passata
 *
 * @param shmid
 * @param file_descriptor
 * @param lunghezza_buffer di lettura
 * @param ordine_matrice passata come parametro
 * @return un puntatore alla cella di memoria condivisa di partenza
 */
int *scrivi_matrice(int shmid, int file_descriptor, int numero_valori, int ordine_matrice){

    int *shared_memory;                                 // puntatore alla cella di memoria condivisa iniziale
    char buf[numero_valori];                            // buffer di lettura
    int i = 0;                                          // variabile utilizzata per la scrittura delle stringhe
    char *righe[ordine_matrice];                        // scrivo nell'array tutte le righe splittate in base al \n
    char *p;                                            // puntatore alla stringa che abbiamo splittato
    int valori[ordine_matrice*ordine_matrice];          // matrice che contiene tutti i valori splittati in base al ;

    lseek(file_descriptor, 0, 0);                       // mi posiziono all'inizio del file
                                                        // leggo tutto il contenuto della matrice
    if (read(file_descriptor, buf, numero_valori-1) == -1){
        print_error("Errore durante la lettura completa del buffer!\n");
        close_all();
    }


    p = strtok (buf, "\n");                             // salvo la prima striga splitta
    while (p != NULL){                                  // faccio un ciclo dalla fine verso l'inizio
        righe[i++] = p;                                 // e salvo all'interno dell'array la stringa letta
        p = strtok (NULL, "\n");                        // dopodiche splitto ancora
    }

    i = 0;                                              // posiziono l'indice dell'array
    for (int riga = 0; riga < ordine_matrice; riga++) {
        p = strtok((righe[riga]), ";");                 // splitto il primo valore che si trova all'interno dell'array delle righe
        valori[i] = atoi(p);                            // salvo il primo valore della riga all'interno dell'array dei valori interi
        while (p != NULL) {
            valori[i++] = atoi(p);                      // procedo con il secondo e cosi via
            p = strtok(NULL, ";");
        }
    }

    shared_memory = (int *) malloc(sizeof(int));        // deve essere dichiarato --> altrimenti segmentation fault

                                                        // mi "attacco" all'area di memoria creata nel main
    if ((shared_memory = (int *) shmat(shmid, 0, 0)) == (void *) -1)
        print_error("Errore durante la creazione del segmento di memoria!\n");

    // scrive nell area di memoria creata

    for (int j = 0; j < ordine_matrice*ordine_matrice; j++)
        *(shared_memory + j) = valori[j];                 // salvo negli indirizzi di memoria tutti gli interi

        memset(buf, 0, sizeof(int)*numero_valori);          // resetto il buffer, potrebbero esserci degli errori nella lettura della seconda matrice

    return shared_memory;
}

/**
 * Funzione che si occupa di moltiplicare una riga per una colonna e che scrive in memoria condivisa il risultato
 *
 * @param shared_memory_a
 * @param shared_memory_b
 * @param sahred_memory_c_molt
 * @param riga_i
 * @param riga_j
 * @param ordine_matrice
 */
void *moltiplica(void *value) {

    int *shared_memory_a_new, *shared_memory_b_new, *shared_memory_c_molt_new;          // siccome le thread condividono area e memoria condivisa dobbiamo salvare il
                                                                                        // nuovo valore del puntatore in un puntatore temporaneo
    int riga, colonna, ordine_matrice, risultato = 0;                                   // variabili temporanee
    message *msg_from_thread = (message *) value;                                       // copio tutto ciò che all'interno della struct passata dalla thread

    riga = msg_from_thread->riga;                                                       // copio in riga il valore della riga da moltiplicare
    colonna = msg_from_thread->colonna;                                                 // copio in colonna il valore da moltiplicare
    ordine_matrice = msg_from_thread->ordine_matrice;                                   // copio in ordine_matrice il valore dell'ordine della matrice

    int righe[ordine_matrice], colonne[ordine_matrice];                                 // creo due vettori dove verranno salvate le righe e le colonne

    shared_memory_a_new = shared_memory_a + (riga*ordine_matrice);                      // mi posiziono nella zona corretta dell'area di memoria

    for (int i = 0; i < ordine_matrice; i++)                                            // salvo all'interno di un array la riga
        righe[i] = *(shared_memory_a_new + i);

    shared_memory_b_new = shared_memory_b +(colonna);                                   // anche qui mi posiziono nella zona corretta

    for (int i = 0; i < ordine_matrice; i++){                                           // salvo all'interno di un array la colonna
        colonne[i] = *shared_memory_b_new;
        shared_memory_b_new += ordine_matrice;
    }

    for (int i = 0; i < ordine_matrice; i++)                                            // eseguo la moltiplicazione sulla matrice
        risultato += (righe[i] * colonne[i]);

    shared_memory_c_molt_new = shared_memory_c_molt + (riga * ordine_matrice) + colonna;// mi posiziono nella locazione di memoria corretta
    *(shared_memory_c_molt_new) = risultato;                                            // scrivo all'interno della memoria il valore

    pthread_exit(NULL);
}

/**
 *
 * @param sharee_memory_a
 * @param shared_memory_b
 * @param shared_memory_c_sum
 * @param riga
 * @param ordine_matrice
 */
void *somma(void *value){

    pthread_mutex_lock(&mutex);                                                 // blocco il mutex delle thread

    int i;                                                                      // contatore usato all'interno dei cicli
    int risultato = 0;                                                          // array che contiene tutti i risultati della matrice
    int *shared_memory_c_molt_new;                                              // puntatore dove salvo il nuovo valore
    int riga, ordine_matrice;                                                   // variabili sulla quale salvo i valori
    message *msg_from_thread = (message *) value;                               // copio tutto ciò che all'interno della struct passata dalla thread

    riga = msg_from_thread->riga;                                               // copio in riga il valore della riga da moltiplicare
    ordine_matrice = msg_from_thread->ordine_matrice;                           // copio in ordine_matrice il valore dell'ordine della matrice

    shared_memory_c_molt_new = shared_memory_c_molt + (riga * ordine_matrice) ; // mi posiziono nell'offset corretto della matrice

    for (i=0; i < ordine_matrice; i++)                                          // per tutta la lunghezza della riga
        risultato += *(shared_memory_c_molt_new + i);                           // sommo i valori

    *shared_memory_c_sum += risultato;                                          // scrivo in memoria il valore corretto

    pthread_mutex_unlock(&mutex);                                               // sblocco il mutex delle thread
}

/**
 * Funzione che si occupa della eliminazione delle risorse quando viene premuto ctrl+c
 *
 */
void close_all(){

    shmctl(shmid_a, IPC_RMID, 0);                                                               // rimuovo tutte le aree di memoria
    shmctl(shmid_b, IPC_RMID, 0);
    shmctl(shmid_c_molt, IPC_RMID, 0);
    shmctl(shmid_c_sum, IPC_RMID, 0);

    pthread_mutex_destroy(&mutex);                                                              // distruzione del mutex

    close(fd_a);                                                                                // chiusura dei file descriptor
    close(fd_b);
    close(fd_c);

    kill(getpid(), 0);                                                                           // killo tutti i processi appartenenti al padre

    print("Terminazione del programma!\n");
    exit(0);
}
