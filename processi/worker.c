//
// Created by michele on 15/05/17.
//

/**
 * Figlio, viene mandato in esecuzione dal padre tramite l'exec.
 *
 * Si occupa di svolgere la moltiplicazione e la somma.
 *
 * @file worker.c
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include "function.h"

#define SEMAFORO 0                                                                              // valore del semaforo inizializzato

int main(int argc, char *argv[]){

    int *shared_memory_a, *shared_memory_b, *shared_memory_c_molt, *shared_memory_c_sum;        // puntatori alle matrici
    int shmid_a, shmid_b, shmid_c_molt, shmid_c_sum, id_mess, sem_id;                           // valore di ritorno della shmget() e msgget() e semget()
    key_t keyA, keyB, keyC_molt, keyC_sum, key_queue, key_sem;                                  // chiavi della creazione della memoria condivisa, e coda di messaggi
    int fd_pipe_read;                                                                           // file descriptor per la lettura dalla pipe -> viene passato
    int riga_i, colonna_j, riga_k;                                                              // numero di riga e di colonna di moltiplcazione, e quella usata nella somma
    char op;                                                                                    // operazione letta dalla pipe
    int ordine;                                                                                 // ordine della matrice
    int error = 0;                                                                              // errore della read
    int numero_processo = 0;                                                                    // numero del processo che viene eseguito dal padre
    message msg_receive;                                                                        // struct dove salvo i valori del messaggio inviato dalla pipe
    queue_message msg;                                                                          // struttura che identifica la coda di messaggi in scrittura

    ///@brief Creazioni delle chaivi univoche tramite ftok(- , -);
    keyA = ftok("/tmp", 'A');                                                                   // inizializzazione chiavi della pipe
    keyB = ftok("/tmp", 'B');
    keyC_molt = ftok("/tmp", 'M');
    keyC_sum = ftok("/tmp", 'S');

    key_queue = ftok("/tmp", 'Q');                                                              // inizializzazione della chiave per la coda di messaggi

    key_sem = ftok("/tmp", 'F');                                                                // inizializzazione della chiave del semaforo intero

    ordine = atoi(argv[1]);                                                                     // ordine della matrice salvato in una nuova variabile
    fd_pipe_read = atoi(argv[2]);                                                               // salvo il valore del file descriptor in lettura che viene passato dalla pipe
    numero_processo = atoi(argv[3]);                                                            // salvo il numero di processo che viene eseguito
    msg.mtype = 1;                                                                              // tipo di messaggio da inviare

    ///@brief Allocazione delle memorie condivise tramite funzioni apposite
    if ((shmid_a = shmget(keyA, ordine*ordine*sizeof(int), 0666)) == -1)                        // prima area di memoria
        print_error("Errore durante la creazione del segmento di memoria in worker.c!\n");

    if ((shared_memory_a = (int *) shmat(shmid_a, NULL, 0)) == (void *) -1)
        print_error("Errore durante l'attach sul segmento di memoria.\n");

    if ((shmid_b = shmget(keyB, ordine*ordine*sizeof(int), 0666)) == -1)                        // seconda area di memoria
        print_error("Errore durante la creazione del segmento di memoria in worker.c!\n");

    if ((shared_memory_b = (int *) shmat(shmid_b, NULL, 0)) == (void *) -1)
        print_error("Errore durante l'attach sul segmento di memoria.\n");

    if ((shmid_c_molt = shmget(keyC_molt, ordine*ordine*sizeof(int), 0666)) == -1)              // terza area di memoria
        print_error("Errore durante la creazione del segmento di memoria in worker.c!\n");

    if ((shared_memory_c_molt = (int *) shmat(shmid_c_molt, NULL, 0)) == (void *) -1)
        print_error("Errore durante l'attach sul segmento di memoria.\n");

    if ((shmid_c_sum = shmget(keyC_sum, sizeof(int), 0666 )) == -1)                             // quarta area di memoria
        print_error("Errore durante la creazione del segmento di memoria in worker.c!\n");

    if ((shared_memory_c_sum = (int *) shmat(shmid_c_sum, NULL, 0)) == (void *) -1)
        print_error("Errore durante l'attach sul segmento di memoria.\n");

    ///@brief Creazione della coda di messaggi
    if ((id_mess = msgget(key_queue, 0666)) == -1)                                              // controllo creazione della coda di messaggi
        print_error("Errore durante la creazione della coda di messaggi.\n");

    ///@brief Creazione del semaforo
    if ((sem_id = semget(key_sem, 1, 0666)) == -1)                                              // creato il semaforo
        print_error("Errore durante la creazione del semaforo in worker.c!\n");

    do {
        ///@brief Lettura dell'operazione da eseguire
        error = read(fd_pipe_read, &msg_receive, sizeof(msg_receive));                          // lettura dalla pipe

        switch (msg_receive.operazione) {                                                       // faccio lo switch in base al caso in cui ci troviamo
            case 'M':                                                                           // siamo nel caso di una MOLTIPLCAZIONE
                ///@brief Eseguiamo una moltiplicazione
                riga_i = msg_receive.riga;                                                      // valore ricevuto tramite PIPE
                colonna_j = msg_receive.colonna;                                                // valore ricevuto tramite PIPE
                moltiplica(shared_memory_a, shared_memory_b, shared_memory_c_molt, riga_i, colonna_j, ordine);
                msg.numero_processo = numero_processo;                                          // numero processo che è stato eseguito
                msgsnd(id_mess, &msg, sizeof(msg) - sizeof(msg.mtype), 1);
                break;
            case 'S':                                                                           // siamo nel caso di una SOMMA
                ///@brief Eseguiamo una somma
                riga_k = msg_receive.riga;                                                      // valore ricevuto tramti PIPE
                sem_dec(sem_id, SEMAFORO);
                somma(shared_memory_c_molt, shared_memory_c_sum, riga_k, ordine);               // sezione critica
                sem_inc(sem_id, SEMAFORO);
                msg.numero_processo = numero_processo;                                          // numero processo che è stato eseguito
                msgsnd(id_mess, &msg, sizeof(msg) - sizeof(msg.mtype), 1);
                break;
            default:                                                                            // siamo nel caso di un FORMATO DI MESSAGIO NON IDONEO
                break;
        }

        if (error == -1)
            print_error("Errore nella lettura della pipe! Chiusura dell'esecuzione.\n");

        ///@brief Usciamo dal ciclo solamente se ci arriva il comando da parte del padre
    } while (msg_receive.operazione != 'E');                                                    // stiamo qui dentro finchè il padre non dice di terminare

    ///@brief Rilascio delle risorse
    shmdt(shmid_a);                                                                             // faccio la detach di tutti i segmenti di memoria che mi sono creato
    shmdt(shmid_b);
    shmdt(shmid_c_molt);
    shmdt(shmid_c_sum);

    return 0;

}
