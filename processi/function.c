//
// Created by michele on 15/05/17.
//

///@file function.c

#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/stat.h>
#include "function.h"

/**
 * Per fare la stampa a video utilizzando la sys_call write su stdout
 *
 * @param string[] puntatore ad una stringa qualsiasi
 */
void print(char *string){
    if((write(1, string, strlen(string))) == -1) {
        close_all();
        exit(1);
    }
}

/**
 * Per fare la stampa a video utilizzando la sys_call write su stderror
 *
 * @param string[] puntatore ad una stringa qualsiasi
 */
void print_error(char *string){
    write(2, string, strlen(string));                                // scrivo su stderror
    close_all();                                                     // chiudo IPC
    exit(0);                                                        // termino programma
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

    if ((shmid = shmget(key, ordine*ordine*sizeof(int), 0666 | IPC_CREAT | IPC_EXCL)) == -1)        // creo l'area di memoria condivisa, con IPC_CREAT
        print_error("Errore durante la creazione del segmento di memoria in worker.c!\n");          // controllo il valore restituito, se -1 errore

    return shmid;                                                                                   // ritorno il valore corretto
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
int *scrivi_matrice(int shmid, int file_descriptor, int ordine_matrice){

    int *shared_memory;                                 // puntatore alla cella di memoria condivisa iniziale
    struct stat file_info;                              // dichiaro la struct per che contiene le informazioni dei file
    fstat(file_descriptor, &file_info);                 // fstat, scrive nella struttura tutte le informazioni dei file
    int numero_valori = file_info.st_size;              // salvo il numero dei byte al suo interno
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

    if(shared_memory == (int *) -1){
        print_error("Errore durante il binding del segmento!");
        return 0;
    }

    // scrive nell area di memoria creata

    for (int j = 0; j < ordine_matrice*ordine_matrice; j++)
        *(shared_memory+j) = valori[j];                 // salvo negli indirizzi di memoria tutti gli interi

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
void moltiplica(int *shared_memory_a, int *shared_memory_b, int *shared_memory_c_molt, int riga_i, int riga_j, int ordine_matrice) {

    int riga[ordine_matrice];                                           // valore della riga
    int colonna[ordine_matrice];                                        // valore della colonna
    int i;                                                              // contatore usato all'interno dei cicli
    int risultato = 0;                                                  // valore (somma) che contiene tutti i risultati della matrice

    shared_memory_a = shared_memory_a + (riga_i*ordine_matrice);        // mi posiziono nella zona corretta dell'area di memoria

    for (i = 0; i < ordine_matrice; i++)                                // salvo all'interno di un array la riga
        riga[i] = *(shared_memory_a + i);


    shared_memory_b = shared_memory_b +(riga_j);                        // anche qui mi posiziono nella zona corretta

    for (i = 0; i < ordine_matrice; i++){                               // salvo all'interno di un array la colonna
        colonna[i] = *shared_memory_b;
        shared_memory_b += ordine_matrice;
    }

    for (i = 0; i < ordine_matrice; i++)                                // eseguo la moltiplicazione sulla matrice
        risultato += (riga[i] * colonna[i]);

    shared_memory_c_molt = shared_memory_c_molt + (riga_i * ordine_matrice) + riga_j;   // mi posiziono nella locazione di memoria corretta
    *(shared_memory_c_molt) = risultato;                                                // scrivo all'interno della memoria il valore
}

/**
 *
 * @param sharee_memory_a
 * @param shared_memory_b
 * @param shared_memory_c_sum
 * @param riga_k
 * @param ordine_matrice
 */
void somma(int *shared_memory_c_molt, int *shared_memory_c_sum, int riga_k, int ordine_matrice){

    int i;                                                                      // contatore usato all'interno dei cicli
    int risultato = 0;                                                          // array che contiene tutti i risultati della matrice

    shared_memory_c_molt = shared_memory_c_molt + (riga_k * ordine_matrice) ;   // mi posiziono nell'offset corretto della matrice

    for (i=0; i < ordine_matrice; i++)                                          // per tutta la lunghezza della riga
        risultato += *(shared_memory_c_molt + i);                                // sommo i valori

    *shared_memory_c_sum += risultato;                                          // scrivo in memoria il valore corretto
}

/**
 * Funzione che riempie la struct message con l'operazione e gli indici passati
 *
 * @param operazione
 * @param riga
 * @param colonna
 * @return
 */
message riempi_struct(char operazione, int riga, int colonna){

    message save;

    switch (operazione){                                            // in base all'operazione inserita decido cosa fare
        case 'M':                                                   // MOLTIPLICAZIONE
            save.operazione = operazione;
            save.riga = riga;
            save.colonna = colonna;
            break;
        case 'S':                                                   // SOMMA
            save.operazione = operazione;
            save.riga = riga;
            save.colonna = riga;
            break;
        case 'E':                                                   // CHIUSURA DEI PROCESSI, DETACH
            save.operazione = operazione;
            save.riga = riga;
            save.colonna = riga;
            break;
        default:                                                    // struttura del messaggio non consentita, ERRORE
            print_error("Operazione non consentita!\n");
            break;
    }

    return save;                                                    // ritorno del valore della struct
}

/**
 * Funzione che fa la wait, si occupa di "decrementare" il valore del semaforo
 *
 * @param sem_id
 * @param sem_numero
 */
void sem_dec(int sem_id, int sem_numero){

    struct sembuf sembuf;

    sembuf.sem_num = (unsigned short) sem_numero;                                       // setto il numero del semaforo che viene passato come parametro
    sembuf.sem_op = -1;                                                                 // decremento il valore del semaforo --> se fosse mutex è come mettere a 0
    sembuf.sem_flg = 0;                                                                 // nessuna flag impostata

    if((semop(sem_id, &sembuf, 1)) == -1) {
        print_error("Errore durante la wait all'interno della funzione sem_dec!\n");    // eseguo la wait
        exit(1);
    }

    return;
}

/**
 * Funzione che si occupa di fare la signal, cioè di "aumentare" il valore del semaforo
 *
 * @param sem_id
 * @param sem_numero
 */
void sem_inc(int sem_id, int sem_numero){

    struct sembuf sembuf;

    sembuf.sem_num = (unsigned short) sem_numero;                                       // setto il numero del semaforo che viene passato come parametro
    sembuf.sem_op = 1;                                                                  // aumento il valore del semaforo --> se fosse mutex è come mettere a 1
    sembuf.sem_flg = 0;                                                                 // nessuna flag impostata

    if((semop(sem_id, &sembuf, 1)) == -1) {
        print_error("Errore durante la wait all'interno della funzione sem_inc!\n");    // eseguo la wait
        exit(1);
    }

    return;
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

    msgctl(id_mess, IPC_RMID, NULL);                                                            // rimuovo la coda di messaggi

    semctl(sem_id, IPC_RMID, 0);                                                                // rimuovo il semaforo

    close(fd_a);                                                                                // chiusura dei file descriptor
    close(fd_b);
    close(fd_c);

    kill(-getpid(), 9);                                                                         // killo tutti i processi appartenenti al padre

    print("Terminazione del programma!\n");
    exit(0);
}
