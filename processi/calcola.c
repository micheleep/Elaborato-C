//
// Created by michele on 09/05/17.
//

/**
 * Padre, si occupa di mandare in esecuzione i figli tramite l'exec.
 *
 * Ha il compito di schedulare e decidere la operazioni da svolgere.
 *
 * @file calcola.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include "function.h"

#define SEMAFORO 0

int main(int argc, char *argv[]) {

    if (argc != 6)                                                                            // controllo del numero corretto degli argomenti passati dall'utente
        print_error("Inserire il numero corretto di argomenti!\nModalità di inserimento : <matA> <matB> <matC> <ordine_matrice> <numero_processi>\n");

    int ordine_mat_a, ordine_mat_b;                                                             // variabile dove salvo l'ordine della matrice a e b
    key_t keyA, keyB, keyC_molt, keyC_sum, key_queue, key_sem;                                  // chiavi della creazione della memoria condivisa, e coda di messaggi
    int *shared_memory_a, *shared_memory_b, *shared_memory_c_molt, *shared_memory_c_sum;        // indirizzo della memoria virtuale delle matrici a, b, c_molt, c_sum
    int pipe_fd[2];                                                                             // dichiarazione della pipe
    int array_pipe[atoi(argv[5])];                                                              // array di pipe grande come il numero dei processi
    char file_descriptor_pipe[10];                                                              // variabile che contiene il file descriptor della pipe
    char *argv_worker[5];                                                                       // argomenti passati al worker
    message msg_send;                                                                           // struct contenente il messaggio da inviare
    queue_message msg;                                                                          // struttura che identifica la coda di messaggi in scrittura
    pid_t pid;                                                                                  // pid del processo
    char value_processo[5];                                                                     // contiene il numero del processo convertito con sprintf che viene eseguito
    int np_terminale = atoi(argv[5]);                                                           // numero di processi inserito dall'utente
    int riga = 0, colonna = 0;                                                                  // riga e colonna che vengono eseguite
    int j = 0;                                                                                  // contatore usato nei cicli for
    int n_operazioni = 0;                                                                       // numero delle operazioni totali che si devono compiere
    char *string;                                                                               // stringa utilizzata per la stampa su file
    int counter = 0;                                                                            // contatore utilizzato per la stampa su file

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

    if ((fd_c = open(argv[3], O_RDWR, 0666)) == -1)                                             // apertura del terzo file in lettura e scrittura
        print_error("Non è possibile aprire o scrivere nel terzo file!\n");

    ordine_mat_a = controllo_matrice(fd_a);                                                     // devono essere quadrate ed uguali fra di loro
    ordine_mat_b = controllo_matrice(fd_b);

    if (ordine_mat_a != ordine_mat_b)                                                           // controllo che le matrici siano uguali
        print_error("Le matrici indicate nei file non sono inserite correttamente, non è possibile svolgere alcuna operazione!\nModificare i parametri inseriti in input e riprovare!\n");

    if(ordine_mat_a != atoi(argv[4]))                                                           // controllo che l'ordine inserito da terminale sia uguale
        print_error("Il valore dell'ordine inserito da terminale non corrisponde con le matrici date!\nModificare il parametri inserito in input e riprovare!\n");

    if(atoi(argv[5]) < 1)                                                                       // controllo che il numero dei processi sia maggiore di 0
        print_error("Il numero di processi deve essere corretto, inserire un numero maggiore di zero!\nModificare il numero di processi inseriti e riprovare!\n");

    ///@brief Creazioni delle chaivi univoche tramite ftok(- , -);
    keyA = ftok("/tmp", 'A');                                                                   // inizializzazione delle chaivi usate per la shared memory
    keyB = ftok("/tmp", 'B');
    keyC_molt = ftok("/tmp", 'M');
    keyC_sum = ftok("/tmp", 'S');

    key_queue = ftok("/tmp", 'Q');                                                              // inizializzazione della chiave per la coda di messaggi

    key_sem = ftok("/tmp", 'F');                                                                // inizializzazione della chiave del semaforo intero

    ///@brief Allocazione delle memorie condivise tramite funzioni apposite
    shmid_a = get_memoria_condivisa_padre(keyA, ordine_mat_a);                                  // creo la memoria condivisa
    shared_memory_a = scrivi_matrice(shmid_a, fd_a, ordine_mat_a);  // scrivo all'interno della memoria condivisa creata

    shmid_b = get_memoria_condivisa_padre(keyB, ordine_mat_b);
    shared_memory_b = scrivi_matrice(shmid_b, fd_b, ordine_mat_b);

    shmid_c_molt = get_memoria_condivisa_padre(keyC_molt, ordine_mat_a);
    shared_memory_c_molt = (int *) shmat(shmid_c_molt, NULL, 0);

    if ((shmid_c_sum = shmget(keyC_sum, sizeof(int), 0666 | IPC_CREAT | IPC_EXCL)) == -1)       // creo una area di memoria per un intero
        print_error("Errore durante la creazione del segmento di memoria in calcola.c!\n");

    shared_memory_c_sum = (int *) shmat(shmid_c_sum, NULL, 0);

    ///@brief Creazione della coda di messaggi
    if ((id_mess = msgget(key_queue, (0666 | IPC_CREAT | IPC_EXCL))) == -1)                     // creo la coda di messaggi
        print_error("Errore durante l'apertura della coda di messaggi in worker.c!\n");

    ///@brief Creazione del semaforo
    if ((sem_id = semget(key_sem, 1, 0666 | IPC_CREAT | IPC_EXCL)) == -1)                       // creato il semaforo
        print_error("Errore durante la creazione del semaforo in calcola.c!\n");

    stato_sem.val = 1;                                                                          // inizializzazione iniziale del valore del semaforo

    if (semctl(sem_id, SEMAFORO, SETVAL, stato_sem) == -1)                                      // inizializzato completamente il semaforo
        print_error("Errore durante l'inizializzazione del semaforo in calcola.c!\n");

    ///@brief Passaggio degli argomenti utlizzati dall'exec
    argv_worker[0] = "worker.x";                                                                // devo passare il programma da eseguire
    argv_worker[1] = argv[4];                                                                   // passo l'ordine della matrice
    argv_worker[4] = NULL;                                                                      // ultimo argomento deve essere sempre NULL

    n_operazioni = ordine_mat_a*ordine_mat_a;                                                   // numero delle operazioni da svolgere
    ///@brief Esecuzione della moltiplicazione eseguita dai processi, vengono creati tramite fork()
    for (j = 0; j < np_terminale; j++) {
        if ((pipe(pipe_fd)) == -1)                                                              // pipe_p_f[0] = lettura - pipe_p_f[1] = scrittura
            print_error("Errore nella creazione della pipe!");

        array_pipe[j] = pipe_fd[1];                                                             // nella posizione del processo metto il file_desccriptor della pipe in write

        sprintf(file_descriptor_pipe, "%i", pipe_fd[0]);                                        // tramite sprintf scrivo nella variabile il valore della pipe in lettura
        argv_worker[2] = file_descriptor_pipe;                                                  // passo il file descriptor in lettura come argomento

        sprintf(value_processo, "%i",j);                                                        // tramite sprintf scrivo nella variabile il valore del processo eseguito
        argv_worker[3] = value_processo;                                                        // passo il valore del processo eseguito
        pid = fork();

        if (pid == -1)                                                                          // la fork non è andata come previsto
            print_error("Fork fallita\n");
        else if (pid == 0) {                                                                    // FIGLIO

            if (execv("worker.x", argv_worker) == -1)                                           // esecuzione del figlio
                print_error("Errore EXEC!\n");
        }
        else {
            print("Figlio ");
            print_integer(j+1);
            print(" sta esegundo la moltiplicazione!\n");
            if (j < ordine_mat_a * ordine_mat_a) {                                              // PADRE
                msg_send = riempi_struct('M', riga, colonna);                                   // scrivo nella struttura l'operazione da eseguire

                if (colonna == (ordine_mat_a - 1)) {                                            // se ho finito la colonna aumento la riga di matA
                    riga++;                                                                     // mi sposto sulla riga di matA
                    colonna = 0;                                                                // mi riposiziono all'inizio di matB
                } else
                    colonna++;                                                                  // mi sposto sulla colonna di matB

                if ((write(array_pipe[j], &msg_send, sizeof(message))) == -1)                   // scrivo nel file descriptor del processo eseguito quello che deve fare
                    print_error("Errore nella scrittura nella pipe per la moltplicazione!");

                n_operazioni--;                                                                 // decremento il numero delle operazioni da svolgere
                msgrcv(id_mess, (void *) &msg, sizeof(msg) - sizeof(msg.mtype), 1, 0);          // ricevo gli elementi di quel processo
            }
            // altrimenti non faccio nulla, nessuna moltiplicazione
        }
    }
    ///@brief Se il numero dei processi è inferiore rispetto al numero delle operazioni ciclo finchè non ho terminato le operazioni
    while (n_operazioni) {                                                                      // finchè ho operazioni da svolgere --> faccio le stesse cose di prima
        msg_send = riempi_struct('M', riga, colonna);                                           // scrivo nella struttura l'operazione da eseguire

        if (colonna == (ordine_mat_a - 1)) {
            riga++;
            colonna = 0;
        }
        else {
            colonna++;
        }

        if ((write(array_pipe[msg.numero_processo], &msg_send, sizeof(message))) == -1)
            print_error("Errore di scrittura nella pipe per la moltiplicazione!");              // scrivo nel file descriptor del processo eseguito quello che deve fare

        print("Figlio in esecuzione : ");
        print_integer(msg.numero_processo + 1);
        print("\n");

        j++;                                                                                    // aumento il valore del processo eseguito
        msgrcv(id_mess, (void *) &msg, sizeof(msg) - sizeof(msg.mtype), 1, 0);                  // ricevo gli elementi di quel processo
        n_operazioni--;                                                                         // decremento il numero delle operazioni
    }

    n_operazioni = ordine_mat_a;                                                                // numero delle operazioni da svolgere --> che sono come l'ordine della matrice
    ///@brief Esecuzione della somma, fatta dagli stessi processi che sono stati creati prima
    if(np_terminale >= n_operazioni) {                                                          // se il numero di processi è superiore alle operazioni da fare

        for (j = 0; j < n_operazioni; j++) {
            msg_send = riempi_struct('S', j, j);                                                // compongo la structo con i valori che mi interessano

            if ((write(array_pipe[j], &msg_send, sizeof(message))) == -1)                       // controllo che non ci siano errori
                print_error("Errore di scrittura nella pipe per la somma!");

            msgrcv(id_mess, (void *) &msg, sizeof(msg) - sizeof(msg.mtype), 1, 0);              // ricevo gli elementi di quel processo
        }

    }
    else {
        for (j = 0; j < np_terminale; j++) {

            msg_send = riempi_struct('S', j, j);                                                // compongo la structo con i valori che mi interessano

            if ((write(array_pipe[j], &msg_send, sizeof(message))) == -1)                       // controllo che non ci siano errori
                print_error("Errore di scrittura nella pipe per la somma!");

            msgrcv(id_mess, (void *) &msg, sizeof(msg) - sizeof(msg.mtype), 1, 0);              // ricevo gli elementi di quel processo
            n_operazioni--;
        }

        while (n_operazioni) {
            msg_send = riempi_struct('S', j, j);                                                // compongo la structo con i valori che mi interessano

            if ((write(array_pipe[msg.numero_processo], &msg_send, sizeof(message))) == -1)     // controllo che non ci siano
                print_error("Errore di scrittura nella pipe per la somma!");

            msgrcv(id_mess, (void *) &msg, sizeof(msg) - sizeof(msg.mtype), 1, 0);              // ricevo gli elementi di quel processo
            j++;                                                                                // aumento la riga e la colonna sulla quale eseguire
            n_operazioni--;                                                                     // decremento le operazioni da svolgere
        }
    }
    print("\nRisultato finale della somma : ");                                                   // stampo il risultato della somma a video
    print_integer(*shared_memory_c_sum);
    print("\n");

    ///@brief Scrittura su file della matrice C derivata dalla moltiplicazione

    string = (char *) malloc(sizeof(char) * 20);                                                // alloco in memoria uno spazio per la strina

    for (j = 0; j < ordine_mat_a*ordine_mat_a ; j++) {
        sprintf(string, "%d", *(shared_memory_c_molt+j));                                       // scrivo la stringa su una variabile temporanea diciamo
        write(fd_c, string, strlen(string));                                                    // scrivo su file §

        if (counter / (ordine_mat_a-1) == 1) {                                                  // stampiamo il valore \n che corrisponde con a capo
            if(write(fd_c, "\n", sizeof(char)) == -1)
                print_error("Problema scrittura su matrice!\n");

            counter = 0;
        }
        else {
            if(write(fd_c, ";", sizeof(char)) == -1)                                             // oppure il ;
                print_error("Problema scrittura su matrice!\n");

            counter++;
        }
    }

    free(string);                                                                               // dealloco lo spazio inizializzato per il puntatore alla stringa

    ///@brief Chiusura di tutti i processi, tramite il carattere E
    for (int i = 0; i < np_terminale; i++) {                                                    // chiude i processi che sono in giro a non fare nulla
        msg_send = riempi_struct('E', i, i);
        // finchè non mando questa operazione i processi non vengono chiusi
        if ((write(array_pipe[i], &msg_send, sizeof(message))) ==-1)                            // invio tramite pipe il messaggio di terminazione
            print_error("Errore nella terminazione dei processi!\n");

    }

    ///@brief Aspetto finchè tutti i processi non vengono chiusi
    for (int i = 0; i < np_terminale ; i++)
        msgrcv(id_mess, (void *) &msg, sizeof(msg) - sizeof(msg.mtype), 2, 0);                  // ricevo gli elementi di quel processo

    ///@brief Rimozione di tutte le IPC
    shmctl(shmid_a, IPC_RMID, 0);                                                               // rimuovo tutte le aree di memoria
    shmctl(shmid_b, IPC_RMID, 0);
    shmctl(shmid_c_molt, IPC_RMID, 0);
    shmctl(shmid_c_sum, IPC_RMID, 0);

    msgctl(id_mess, IPC_RMID, NULL);                                                            // rimuovo la coda di messaggi

    semctl(sem_id, IPC_RMID, 0);                                                                // rimuovo il semaforo

    close(fd_a);                                                                                // chiusura dei file descriptor
    close(fd_b);
    close(fd_c);

    return 0;
}
