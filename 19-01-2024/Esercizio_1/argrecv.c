/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere un secondo programma argrecv che preso in input l'output di argsend esegua il comando
con gli argomenti passati a argsend. Per esempio:
$ ./argsend ls -l /tmp | ./argrecv
total 8988
-rw-r--r-- 1 renzo         renzo         150532 Jan  9 16:57 ....
.....
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define MAX_ARGS 256
#define BUF_SIZE 65536

int main(void){
    static char buf[BUF_SIZE];
    size_t total = 0;

    /* l'input puo' arrivare a pezzi da una pipe: serve accumularlo tutto
     * prima di poterlo spezzare in stringhe sui terminatori '\0' */
    ssize_t n;
    while((n = read(STDIN_FILENO, buf + total, sizeof(buf) - total)) > 0){
        total += (size_t)n;
        if(total >= sizeof(buf)){
            fprintf(stderr, "argrecv: input troppo grande\n");
            exit(EXIT_FAILURE);
        }
    }
    if(n == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }

    if(total == 0){
        fprintf(stderr, "argrecv: nessun comando ricevuto\n");
        exit(EXIT_FAILURE);
    }

    char *args[MAX_ARGS];
    int nargs = 0;
    size_t pos = 0;
    while(pos < total && nargs < MAX_ARGS - 1){
        args[nargs++] = buf + pos;
        pos += strlen(buf + pos) + 1;
    }
    args[nargs] = NULL;

    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
}
