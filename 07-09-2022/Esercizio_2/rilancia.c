/**
 Se si esegue il programa dell'esercizio 1 in questo modo:
rilancia cat /etc/hostname
il comando (cat) viene eseguito ripetutamente all'infinito.
Modificare il programma rilancia per fare in modo che se l'esecuzione del programma (in questo caso
cat /etc/hostname) dura meno di un secondo non si proceda alla riattivazione.
 */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<time.h>

int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s <program> [args...]\n", argv[0]);
        return -1;
    }

    int should_relaunch;

    do{
        struct timespec start, end;

        if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
            /* CLOCK_MONOTONIC non e' influenzato da modifiche all'orologio di
               sistema: adatto a misurare un intervallo di tempo */
            perror("Error in clock_gettime");
            return -1;
        }

        pid_t pid = fork();

        if(pid == -1){
            perror("Error in fork");
            return -1;
        }

        if(pid == 0){
            execvp(argv[1], &argv[1]);
            perror("Error in execvp");
            _exit(-1);
        }

        int status;

        if(waitpid(pid, &status, 0) == -1){
            perror("Error in waitpid");
            return -1;
        }

        if(clock_gettime(CLOCK_MONOTONIC, &end) == -1){
            perror("Error in clock_gettime");
            return -1;
        }

        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        should_relaunch = WIFEXITED(status) && WEXITSTATUS(status) == 0 && elapsed >= 1.0;
        /* Oltre alle condizioni dell'esercizio 1, il rilancio avviene solo se
           l'esecuzione e' durata almeno un secondo, per evitare loop troppo rapidi
           su comandi che terminano quasi istantaneamente (es. cat di un file piccolo) */

    }while(should_relaunch);

    return 0;
}
