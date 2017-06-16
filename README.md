# Elaborato-C

## Descrizione
Semplice implementazione di un prodotto tra matrici in C.
Il tutto viene implementato e la moltiplicazione vengono eseguite in due modalità :

      - processi
      - thread

## Compilazione
Compilare con make ed eseguire come descritto successivamente.
Eseguire " make doc " per generare la documentazione corretta.
Eseguire " make clean " per rimuovere i file oggetti e gli eseguibili, rimuove inoltre la documentazione doxygen.

## Esecuzione
Eseguire con :

                - ./calcola.x matA matB matC 4 6
                - ./calcola.x matA matB matC 4
### Modalità di Esecuzione
I primi tre parametri sono i file in formato csv delle matrici, il quarto elemento l'ordine della matrice ed il quinto il numero dei processi.
Per quanto riguarda la compilazione del programma con thread non occorre inserire il quinto parametro.
