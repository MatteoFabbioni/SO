/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere un programma fifotext che:
* crei una named pipe (FIFO) al pathname indicato come primo (e unico) argomento.
* apra la named pipe in lettura
* stampi ogni riga di testo ricevuta
* se la named pipe viene chiusa la riapra
* se riceve la riga "FINE" termini cancellando la named pipe.

Esempio:
fifotext /tmp/ff
....
se in un altra shell si fornisce il comando: "echo ciao > /tmp/ff", fifotext stampa ciao e rimane in attesa
(questo esperimento si può provare più volte). Con il comando "echo FINE > /tmp/ff" fifotext termina.
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<errno.h>

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <fifo_pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *fifo_path = argv[1];

    if(mkfifo(fifo_path, 0666) == -1){
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    int finished = 0;
    while(!finished){
        /* l'apertura in sola lettura si blocca finche' uno scrittore non apre
         * la fifo: e' il comportamento voluto per attendere il prossimo scrittore */
        FILE *fp = fopen(fifo_path, "r");
        if(fp == NULL){
            perror("fopen");
            unlink(fifo_path);
            exit(EXIT_FAILURE);
        }

        char line[4096];
        while(fgets(line, sizeof(line), fp) != NULL){
            size_t len = strlen(line);
            if(len > 0 && line[len - 1] == '\n'){
                line[len - 1] = '\0';
            }

            if(strcmp(line, "FINE") == 0){
                finished = 1;
                break;
            }

            printf("%s\n", line);
            fflush(stdout);
        }

        fclose(fp);
    }

    if(unlink(fifo_path) == -1){
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    return 0;
}
