/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere un programma argsend che converta i parametri del programma (da argv[1] in poi) in una
unica sequenza di caratteri: vengono concatenati i parametri (compreso il terminatore della stringa).
Esempio di funzionamento:
$ ./argsend ls -l /tmp | od -c
0000000 l s \0 - l \0 / t m p \0
0000013
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for(int i = 1; i < argc; i++){
        /* +1 per includere il terminatore '\0' nella scrittura, come richiesto */
        size_t len = strlen(argv[i]) + 1;
        if(write(STDOUT_FILENO, argv[i], len) == -1){
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
