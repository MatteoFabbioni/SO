/**
 Scrivere un programma C rilancia: che esegua il programma indicato in argv[1] con i relativi parametri
(in argv[2] e seguenti):
es: rilancia tr a-z A-Z
esegue il comando tr coi parametri.
Se il programma lanciato termina senza errori (non per colpa di un segnale e con valore di ritorno 0)
rilancia deve rieseguire il programma (nell'esempio tr) con i medesimi parametri.
 */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s <program> [args...]\n", argv[0]);
        return -1;
    }

    int should_relaunch;

    do{
        pid_t pid = fork();

        if(pid == -1){
            perror("Error in fork");
            return -1;
        }

        if(pid == 0){
            execvp(argv[1], &argv[1]);
            /* Se execvp ritorna, l'exec è fallito: bisogna terminare il figlio
               con _exit per non eseguire il resto del programma del padre */
            perror("Error in execvp");
            _exit(-1);
        }

        int status;

        if(waitpid(pid, &status, 0) == -1){
            perror("Error in waitpid");
            return -1;
        }

        should_relaunch = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        /* Il rilancio avviene solo se il processo e' terminato normalmente
           (non ucciso da un segnale) e con codice di uscita 0 */

    }while(should_relaunch);

    return 0;
}
