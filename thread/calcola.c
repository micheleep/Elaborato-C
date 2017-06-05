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

    int ordine_mat_a, ordine_mat_b;                                                             // variabile dove salvo l'ordine della matrice a e b
    key_t keyA, keyB, keyC_molt, keyC_sum, key_queue, key_sem;                                  // chiavi della creazione della memoria condivisa, e coda di messaggi
    int riga = 0, colonna = 0;                                                                  // riga e colonna che vengono eseguite
    pthread_t threads[atoi(argv[4])*atoi(argv[4])];                                             // creo un vettore di n thread quante il quadrato dell'ordine delle matrici
    int counter = 0;                                                                            // contatore delle thread
    int value = 0;                                                                              // valore di ritorno della creazione delle thread --> errore
    char *string[5];                                                                            // stringa utilizzata per la stampa su file

    signal(SIGINT, close_all);

    /**@brief Controlli eseguiti sul main :
    *     - valore corretto degli argomenti
    *     - controllo apertura dei file
    *     - controllo ordine della matrice tramite funzione
    *     - controllo che il numero dei processi non sia inferiore a uno
    */

     if (argc != 5)                                                                             // controllo del numero corretto degli argomenti passati dall'utente
        print_error("Inserire il numero corretto di argomenti!\nModalità di inserimento : <matA> <matB> <matC> <ordine_matrice> <numero_processi>\n");

    if ((fd_a = open(argv[1], O_RDONLY, 0644)) == -1)                                           // apertura del primo file in sola lettura
        print_error("Non è possibile aprire il primo file!\n");

    if ((fd_b = open(argv[2], O_RDONLY, 0644)) == -1)                                           // apertura del secondo file in sola lettura
        print_error("Non è possibile aprire il secondo file!\n");

    if ((fd_c = open(argv[3], O_RDWR, 0666)) == -1)                                             // apertura del terzo file in lettura e scrittura
        print_error("Non è possibile aprire o scrivere nel terzo file!\n");

    ordine_mat_a = controllo_matrice(fd_a);                                                     // devono essere quadrate ed uguali fra di loro
    ordine_mat_b = controllo_matrice(fd_b);

    if (ordine_mat_a != ordine_mat_b) {                                                         // controllo che le matrici siano uguali
        print("Le matrici indicate nei file non sono inserite correttamente, non è possibile svolgere alcuna operazione!\n");
        print("Modificare i parametri inseriti in input e riprovare!\n");
        return 0;
    }

    if(ordine_mat_a != atoi(argv[4])) {                                                         // controllo che l'ordine inserito da terminale sia uguale
        print("Il valore dell'ordine inserito da terminale non corrisponde con le matrici date!\n");
        print("Modificare il parametri inserito in input e riprovare!\n");
        return 0;
    }

    ///@brief Creazioni delle chaivi univoche tramite ftok(- , -);
    keyA = ftok("/tmp", 'A');                                                                   // inizializzazione delle chaivi usate per la shared memory
    keyB = ftok("/tmp", 'B');
    keyC_molt = ftok("/tmp", 'M');
    keyC_sum = ftok("/tmp", 'S');

    key_sem = ftok("/tmp", 'F');                                                                // inizializzazione della chiave del semaforo intero

    ///@brief Allocazione delle memorie condivise tramite funzioni apposite
    shmid_a = get_memoria_condivisa_padre(keyA, ordine_mat_a);                                  // creo la memoria condivisa
    shared_memory_a = scrivi_matrice(shmid_a, fd_a, conta_valori_matrice(fd_a), ordine_mat_a);  // scrivo all'interno della memoria condivisa creata

    shmid_b = get_memoria_condivisa_padre(keyB, ordine_mat_b);
    shared_memory_b = scrivi_matrice(shmid_b, fd_b, conta_valori_matrice(fd_b), ordine_mat_b);

    shmid_c_molt = get_memoria_condivisa_padre(keyC_molt, ordine_mat_a);
    shared_memory_c_molt = (int *) shmat(shmid_c_molt, NULL, 0);


    if ((shmid_c_sum = shmget(keyC_sum, sizeof(int), 0666 | IPC_CREAT | IPC_EXCL)) == -1){      // creo una area di memoria per un intero
        print_error("Errore durante la creazione del segmento di memoria in calcola.c!\n");
        close_all();
        return 0;
    }
    shared_memory_c_sum = (int *) shmat(shmid_c_sum, NULL, 0);

    pthread_mutex_init(&mutex, NULL);                                                           // inizializzazione del mutex

    ///@brief Creazione delle thread per la parte di moltiplicazione
    for (riga = 0; riga < ordine_mat_a; riga++){                                                // ciclo sulle righe
        for (colonna = 0; colonna < ordine_mat_b; colonna++) {                                  // ciclo sulle colonne
            print("Creo la thread ");
            print_integer(counter+1);
            print(" per la moltiplicazione!\n");
            message *msg_to_thread = (message *) malloc(sizeof(message));                       // operazioni che devono essere effettuate dalle thread

            msg_to_thread->ordine_matrice = ordine_mat_a;
            msg_to_thread->colonna = colonna;
            msg_to_thread->riga = riga;

            value = pthread_create(&threads[counter], NULL, moltiplica, msg_to_thread);         // creo la thread
            if (value == -1)
                print_error("Errore nella creazione della thread!");

            counter++;
        }
    }

    print("\n");

    ///@brief Creazione delle thread per la parte della somma
    for (int i = 0; i < ordine_mat_a; i++) {
        print("Creo la thread ");
        print_integer(i+1);
        print(" per la somma!\n");

        message *msg_to_thread = (message *) malloc(sizeof(message));                           // passaggio dei valori corretti

        msg_to_thread->ordine_matrice = ordine_mat_a;
        msg_to_thread->riga = i;
        msg_to_thread->colonna = i;

        value = pthread_create(&threads[i], NULL, somma, msg_to_thread);                        // creo la thread che deve essere eseguita

        if (value == -1)
            print_error("Errore nella creazione delle thread per la somma!\n");

    }

    ///@brief Aspetto che tutte le thread abbiano finito l'esecuzione
    for (int i = 0; i < ordine_mat_a; i++)
        pthread_join(threads[i], NULL);                                                         // aspetto che tutte le thread finiscano con l'esecuzione

    ///@brief Stampo a video il valore della somma
    print("\nValore della somma : ");
    print_integer(*shared_memory_c_sum);
    print("\n");

    ///@brief Scrittura su file della matrice C derivata dalla moltiplicazione
    for (int j = 0, counter = 0; j < ordine_mat_a*ordine_mat_a ; j++) {
        sprintf(string, "%d", *(shared_memory_c_molt+j));                                       // scrivo la stringa su una variabile temporanea diciamo
        write(fd_c, string, strlen(string));                                                    // scrivo su file

        if (counter / (ordine_mat_a-1) == 1) {                                                  // stampiamo il valore \n che corrisponde con a capo
            write(fd_c, "\n", sizeof(char));
            counter = 0;
        }
        else {
            write(fd_c, ";", sizeof(char));                                                     // oppure il ;
            counter++;
        }
    }

    pthread_mutex_destroy(&mutex);                                                              // distruzione del mutex

    ///@brief Rimozione di tutte le IPC
    shmctl(shmid_a, IPC_RMID, 0);                                                               // rimuovo tutte le aree di memoria
    shmctl(shmid_b, IPC_RMID, 0);
    shmctl(shmid_c_molt, IPC_RMID, 0);
    shmctl(shmid_c_sum, IPC_RMID, 0);

    close(fd_a);                                                                                // chiusura dei file descriptor
    close(fd_b);
    close(fd_c);

    pthread_exit(NULL);                                                                         // terminazione della "thread" del processo padre
                                                                                                // se termina cosi, le altre thread continuano con l'esecuzione
}
