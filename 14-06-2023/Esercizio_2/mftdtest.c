/**
Esercizio 2: Linguaggio C: 10 punti
Estendere l'esercizio 1 in modo che gestisca molteplici timer:
$ mftdtest 2,1.1,a 2,1.2,b 3,0.5,c
0.500255 c
1.000232 c
1.100231 a
1.200266 b
1.500245 c
2.200212 a
2.400255 b
*/

#include<stdio.h>
/* Fornisce printf(), perror(), fprintf() */

#include<stdlib.h>
/* Fornisce exit(), strtol(), strtod() */

#include<string.h>
/* Fornisce strtok() */

#include<sys/timerfd.h>
/* Fornisce timerfd_create(), timerfd_settime() */

#include<unistd.h>
/* Fornisce read(), close() */

#include<poll.h>
/* Fornisce poll(): serve per attendere su molteplici timerfd contemporaneamente */

#include<stdint.h>
/* Fornisce uint64_t */

typedef struct {
    int fd;
    long remaining;
    /* iterazioni ancora da stampare per questo timer, decrementate a ogni scadenza */
    double interval;
    double elapsed;
    char *string;
} timer_t_info;

int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr, "Uso: %s <iterazioni,intervallo,stringa> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n_timers = argc - 1;
    timer_t_info *timers = malloc(n_timers * sizeof(timer_t_info));
    struct pollfd *pfds = malloc(n_timers * sizeof(struct pollfd));
    if(timers == NULL || pfds == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < n_timers; i++){
        char *tok_iter = strtok(argv[i + 1], ",");
        char *tok_interval = strtok(NULL, ",");
        char *tok_string = strtok(NULL, ",");

        if(tok_iter == NULL || tok_interval == NULL || tok_string == NULL){
            fprintf(stderr, "Formato parametro non valido: %s\n", argv[i + 1]);
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

        struct itimerspec its;
        its.it_value.tv_sec = sec;
        its.it_value.tv_nsec = nsec;
        its.it_interval.tv_sec = sec;
        its.it_interval.tv_nsec = nsec;

        if(timerfd_settime(tfd, 0, &its, NULL) == -1){
            perror("timerfd_settime");
            exit(EXIT_FAILURE);
        }

        timers[i].fd = tfd;
        timers[i].remaining = iterations;
        timers[i].interval = interval;
        timers[i].elapsed = 0.0;
        timers[i].string = tok_string;

        pfds[i].fd = tfd;
        pfds[i].events = POLLIN;
    }

    int active = n_timers;
    /* numero di timer che hanno ancora iterazioni da stampare: quando arriva a 0 si termina */

    while(active > 0){
        if(poll(pfds, n_timers, -1) == -1){
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < n_timers; i++){
            if(timers[i].remaining == 0){
                continue;
                /* timer già esaurito: il suo fd resta nell'array per semplicità ma va ignorato */
            }

            if(pfds[i].revents & POLLIN){
                uint64_t expirations;
                ssize_t n = read(pfds[i].fd, &expirations, sizeof(expirations));
                if(n != sizeof(expirations)){
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                timers[i].elapsed += timers[i].interval;
                printf("%f %s\n", timers[i].elapsed, timers[i].string);

                timers[i].remaining--;
                if(timers[i].remaining == 0){
                    pfds[i].fd = -1;
                    /* fd negativo: poll() lo ignora, evitando ulteriori risvegli per questo timer */
                    active--;
                }
            }
        }
    }

    for(int i = 0; i < n_timers; i++){
        close(timers[i].fd);
    }
    free(timers);
    free(pfds);

    return 0;
}
