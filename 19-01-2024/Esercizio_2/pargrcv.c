/**
Esercizio 2: Linguaggio C: 10 punti
Scrivere un programma pargrcv che crei una named pipe (il pathname è passato come parametro) e
quando si ridireziona nella named pipe la sequenza di caratteri creata da argsend dell'esercizio 1,
pargrcv deve eseguire il comando.
$ ./pargrcv /tmp/mypipe
crea la named pipe /tmp/mypipe e si mette in attesa.
Da un'altro terminale il comando:
$ ./argsend ls -l /tmp > /tmp/mypipe
fa eseguire il comando "ls -l /" a pargrcv
 */
#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<errno.h>

#define MAX_ARGS 256
#define BUF_SIZE 65536

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <pipe_pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *pipe_path = argv[1];

    if(mkfifo(pipe_path, 0666) == -1){
        if(errno != EEXIST){
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    /* l'open in lettura si blocca finche' un writer non apre la pipe:
     * e' questo il "mettersi in attesa" richiesto dall'esercizio */
    int fd = open(pipe_path, O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }

    static char buf[BUF_SIZE];
    size_t total = 0;
    ssize_t n;
    while((n = read(fd, buf + total, sizeof(buf) - total)) > 0){
        total += (size_t)n;
        if(total >= sizeof(buf)){
            fprintf(stderr, "pargrcv: input troppo grande\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    if(n == -1){
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);

    if(total == 0){
        fprintf(stderr, "pargrcv: nessun comando ricevuto\n");
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
