//
// Created by michele on 26/05/17.
//

/**
 * IMPLEMENTAZIONE CON THREAD
 *
 * Moltiplicazione e somma tramite thread.
 *
 * @file calcola.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "function.h"

int main(int argc, char *argv[]) {

    if (argc != 5)                                                                             // controllo del numero corretto degli argomenti passati dall'utente
       print_error("Inserire il numero corretto di argomenti!\nModalità di inserimento : <matA> <matB> <matC> <ordine_matrice> <numero_processi>\n");

    int ordine_mat_a, ordine_mat_b;                                                             // variabile dove salvo l'ordine della matrice a e b
    key_t keyA, keyB, keyC_molt, keyC_sum, key_queue, key_sem;                                  // chiavi della creazione della memoria condivisa, e coda di messaggi
    int riga = 0, colonna = 0;                                                                  // riga e colonna che vengono eseguite
    pthread_t threads[atoi(argv[4])*atoi(argv[4])];                                             // creo un vettore di n thread quante il quadrato dell'ordine delle matrici
    int counter = 0;                                                                            // contatore delle thread
    int value = 0;                                                                              // valore di ritorno della creazione delle thread --> errore
    char *string;                                                                               // stringa utilizzata per la stampa su file
    message msg_to_thread[atoi(argv[4])*atoi(argv[4])];                                         // operazioni che devono essere effettuate dalle thread
    signal(SIGINT, close_all);

    /**@brief Controlli eseguiti sul main :
    *     - valore corretto degli argomenti
    *     - controllo apertura dei file
    *     - controllo ordine della matrice tramite funzione
    *     - controllo che il numero dei processi non sia inferiore a uno
    */

    if ((fd_a = open(argv[1], O_RDONLY, 0644)) == -1)                                           // apertura del primo file in sola lettura
        print_error("Non è possibile aprire il primo file!\n");

    if ((fd_b = open(argv[2], O_RDONLY, 0644)) == -1)                                           // apertura del secondo file in sola lettura
        print_error("Non è possibile aprire il secondo file!\n");

    if ((fd_c = open(argv[3], O_RDWR | O_TRUNC , 0666)) == -1)                                             // apertura del terzo file in lettura e scrittura
        print_error("Non è possibile aprire o scrivere nel terzo file!\n");

    ordine_mat_a = controllo_matrice(fd_a);                                                     // devono essere quadrate ed uguali fra di loro
    ordine_mat_b = controllo_matrice(fd_b);

    if (ordine_mat_a != ordine_mat_b)                                                           // controllo che le matrici siano uguali
        print_error("Le matrici indicate nei file non sono inserite correttamente, non è possibile svolgere alcuna operazione!\nModificare i parametri inseriti in input e riprovare!\n");

    if(ordine_mat_a != atoi(argv[4]))                                                           // controllo che l'ordine inserito da terminale sia uguale
        print_error("Il valore dell'ordine inserito da terminale non corrisponde con le matrici date!\nModificare il parametri inserito in input e riprovare!\n");

    ///@brief Creazione dell'area di memoria dove scrivo le matrici.
    shared_memory_a = (int *) malloc(sizeof(int) * ordine_mat_a * ordine_mat_a);                // alloco lo spazio per le matrici, successivamente ci scrivo all'interno
    shared_memory_b = (int *) malloc(sizeof(int) * ordine_mat_b * ordine_mat_b);
    shared_memory_c_molt = (int *) malloc(sizeof(int) * ordine_mat_a * ordine_mat_b);
    shared_memory_c_sum = (int *) malloc(sizeof(int));

    scrivi_matrice(shared_memory_a, fd_a, ordine_mat_a);                                        // scrivo all'interno della memoria condivisa creata
    scrivi_matrice(shared_memory_b, fd_b, ordine_mat_b);

    pthread_mutex_init(&mutex, NULL);                                                           // inizializzazione del mutex

    ///@brief Creazione delle thread per la parte di moltiplicazione
    for (riga = 0; riga < ordine_mat_a; riga++){                                                // ciclo sulle righe
        for (colonna = 0; colonna < ordine_mat_b; colonna++) {                                  // ciclo sulle colonne
            print("Creo la thread ");
            print_integer(counter+1);                           // TODO coredump here
            print(" per la moltiplicazione!\n");

            msg_to_thread[counter].ordine_matrice = ordine_mat_a;
            msg_to_thread[counter].colonna = colonna;
            msg_to_thread[counter].riga = riga;

            value = pthread_create(&threads[counter], NULL, moltiplica, &msg_to_thread[counter]);         // creo la thread

            if (value == -1)
                print_error("Errore nella creazione della thread!");

            counter++;
        }
    }

    print("\n");

    ///@brief Aspetto che le thread per la moltiplicazione abbiano finito l'esecuzione
    for (int i = 0; i < ordine_mat_a*ordine_mat_b; i++)
        pthread_join(threads[i], NULL);                                                         // aspetto che tutte le thread finiscano con l'esecuzione

    ///@brief Creazione delle thread per la parte della somma
    for (int i = 0; i < ordine_mat_a; i++) {
        print("Creo la thread ");
        print_integer(i+1);
        print(" per la somma!\n");

        msg_to_thread[i].ordine_matrice = ordine_mat_a;
        msg_to_thread[i].riga = i;
        msg_to_thread[i].colonna = i;

        value = pthread_create(&threads[i], NULL, somma, &msg_to_thread[i]);                        // creo la thread che deve essere eseguita

        if (value == -1)
            print_error("Errore nella creazione delle thread per la somma!\n");

    }

    ///@brief Aspetto che le thread per la somma abbiano finito l'esecuzione
    for (int i = 0; i < ordine_mat_a; i++)\
        pthread_join(threads[i], NULL);                                                         // aspetto che tutte le thread finiscano con l'esecuzione

    ///@brief Stampo a video il valore della somma
    print("\nValore della somma : ");
    print_integer(*shared_memory_c_sum);
    print("\n");

    ///@brief Scrittura su file della matrice C derivata dalla moltiplicazione

    string = (char *) malloc(sizeof(char) * 20);                                                // alloco in memoria uno spazio per la strina

    for (int j = 0, counter = 0; j < ordine_mat_a*ordine_mat_a ; j++) {
        sprintf(string, "%d", *(shared_memory_c_molt+j));                                       // scrivo la stringa su una variabile temporanea diciamo
        write(fd_c, string, strlen(string));                                                    // scrivo su file

        if (counter / (ordine_mat_a-1) == 1) {                                                  // stampiamo il valore \n che corrisponde con a capo
            if(write(fd_c, "\n", sizeof(char)) == -1)
                print_error("Problema nella scrittura su file!\n");

            counter = 0;
        }
        else {
            if(write(fd_c, ";", sizeof(char)) == -1)                                            // oppure il ;
                print_error("Problema nella scrittura su file!\n");

            counter++;
        }
    }

    free(string);                                                                               // dealloco lo spazio inizializzato per il puntatore alla stringa

    pthread_mutex_destroy(&mutex);                                                              // distruzione del mutex

    ///@brief Chiusura file descriptor e libero la memoria                                      // chiusura dei file descriptor
    close(fd_a),
    close(fd_b);
    close(fd_c);

    free(shared_memory_a);
    free(shared_memory_b);
    free(shared_memory_c_molt);
    free(shared_memory_c_sum);

    pthread_exit(NULL);                                                                         // terminazione della "thread" del processo padre
}
