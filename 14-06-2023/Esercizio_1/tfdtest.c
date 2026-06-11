/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Facendo uso dei timerfd (vedi timerfd_create) scrivere un programma che stampi una stringa a
intervalli regolari. (il parametro ha tre campi separati da virgola: il numero di iterazioni, l'intervallo fra
iterazione e la stringa da salvare:
tfdtest 4,1.1,ciao
deve stampare ciao quattro volte, rispettivamente dopo 1.1 secondi, 2.2 secondi, 3.3 secondi 4.4
secondi e terminare. L'esecuzione dovrebbe essere simile alla seguente:
$ tfdtest 4,1.1,ciao
1.100267 ciao
2.200423 ciao
3.300143 ciao
4.400053 ciao
*/

#include<stdio.h>
/* Fornisce printf(), perror(), fprintf() */

#include<stdlib.h>
/* Fornisce exit(), strtol(), strtod() */

#include<string.h>
/* Fornisce strtok(), strlen() */

#include<sys/timerfd.h>
/* Fornisce timerfd_create(), timerfd_settime() */

#include<unistd.h>
/* Fornisce read(), close() */

#include<stdint.h>
/* Fornisce uint64_t */

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "Uso: %s <iterazioni,intervallo,stringa>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *param = argv[1];
    /* strtok modifica la stringa: lavoriamo su argv[1] perché non serve preservarlo */

    char *tok_iter = strtok(param, ",");
    char *tok_interval = strtok(NULL, ",");
    char *tok_string = strtok(NULL, ",");

    if(tok_iter == NULL || tok_interval == NULL || tok_string == NULL){
        fprintf(stderr, "Formato parametro non valido, atteso: iterazioni,intervallo,stringa\n");
        exit(EXIT_FAILURE);
    }

    long iterations = strtol(tok_iter, NULL, 10);
    double interval = strtod(tok_interval, NULL);

    if(iterations <= 0 || interval <= 0){
        fprintf(stderr, "Numero di iterazioni e intervallo devono essere positivi\n");
        exit(EXIT_FAILURE);
    }

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(tfd == -1){
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    time_t sec = (time_t) interval;
    long nsec = (long) ((interval - sec) * 1e9);
    /* timerfd_settime vuole un intervallo espresso come secondi + nanosecondi separati */

    struct itimerspec its;
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = nsec;
    its.it_interval.tv_sec = sec;
    its.it_interval.tv_nsec = nsec;
    /* it_interval == it_value: il timer si riarma automaticamente allo stesso intervallo */

    if(timerfd_settime(tfd, 0, &its, NULL) == -1){
        perror("timerfd_settime");
        close(tfd);
        exit(EXIT_FAILURE);
    }

    double elapsed = 0.0;

    for(long i = 0; i < iterations; i++){
        uint64_t expirations;
        ssize_t n = read(tfd, &expirations, sizeof(expirations));
        if(n != sizeof(expirations)){
            perror("read");
            close(tfd);
            exit(EXIT_FAILURE);
        }

        elapsed += interval;
        printf("%f %s\n", elapsed, tok_string);
    }

    close(tfd);

    return 0;
}
