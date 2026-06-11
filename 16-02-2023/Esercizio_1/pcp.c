/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Fare un programma di copia parallelo di file.
    pcp file1 file2
pcp deve creare due processi figli; mentre un processo copia la prima meta' del file, l'altro deve
copiare l'altra meta'.
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX/GNU necessarie per pread()/pwrite() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 4096

/* Copia esattamente 'len' byte dal file 'src_fd' al file 'dst_fd' a partire
   dall'offset 'start' in entrambi i file. pread/pwrite non spostano l'offset
   condiviso del file descriptor, quindi i due processi (che hanno ciascuno
   la propria copia del fd dopo la fork) possono operare in sicurezza sulla
   stessa coppia di file aperti senza interferire tra loro */
void copy_range(int src_fd, int dst_fd, off_t start, off_t len){
    char buf[BUF_SIZE];
    off_t done = 0;

    while(done < len){
        size_t to_read = BUF_SIZE;
        if((off_t)to_read > len - done)
            to_read = (size_t)(len - done);

        ssize_t n = pread(src_fd, buf, to_read, start + done);
        if(n == -1){
            perror("pread");
            exit(EXIT_FAILURE);
        }
        if(n == 0)
            break;
        /* EOF prima del previsto: non dovrebbe accadere se len è corretto */

        ssize_t w = pwrite(dst_fd, buf, (size_t)n, start + done);
        if(w == -1){
            perror("pwrite");
            exit(EXIT_FAILURE);
        }

        done += w;
    }
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s file1 file2\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_path = argv[1];
    const char *dst_path = argv[2];

    struct stat st;
    if(stat(src_path, &st) == -1){
        perror("stat");
        return EXIT_FAILURE;
    }
    if(!S_ISREG(st.st_mode)){
        fprintf(stderr, "Error: '%s' non e' un file regolare\n", src_path);
        return EXIT_FAILURE;
    }

    int src_fd = open(src_path, O_RDONLY);
    if(src_fd == -1){
        perror("open source");
        return EXIT_FAILURE;
    }

    int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dst_fd == -1){
        perror("open destination");
        close(src_fd);
        return EXIT_FAILURE;
    }

    off_t size = st.st_size;
    off_t half = size / 2;

    /* Crea il file di destinazione della dimensione finale prima di scrivere:
       evita che pwrite() su una regione oltre l'attuale EOF lasci un buco
       inatteso e garantisce che la dimensione finale sia corretta anche se
       il secondo figlio scrive prima del primo */
    if(ftruncate(dst_fd, size) == -1){
        perror("ftruncate");
        close(src_fd);
        close(dst_fd);
        return EXIT_FAILURE;
    }

    pid_t pid1 = fork();
    if(pid1 == -1){
        perror("fork");
        close(src_fd);
        close(dst_fd);
        return EXIT_FAILURE;
    }

    if(pid1 == 0){
        copy_range(src_fd, dst_fd, 0, half);
        close(src_fd);
        close(dst_fd);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if(pid2 == -1){
        perror("fork");
        close(src_fd);
        close(dst_fd);
        return EXIT_FAILURE;
    }

    if(pid2 == 0){
        copy_range(src_fd, dst_fd, half, size - half);
        close(src_fd);
        close(dst_fd);
        exit(EXIT_SUCCESS);
    }

    close(src_fd);
    close(dst_fd);

    int status;
    int exit_code = EXIT_SUCCESS;

    if(waitpid(pid1, &status, 0) == -1){
        perror("waitpid");
        exit_code = EXIT_FAILURE;
    } else if(!WIFEXITED(status) || WEXITSTATUS(status) != 0){
        exit_code = EXIT_FAILURE;
    }

    if(waitpid(pid2, &status, 0) == -1){
        perror("waitpid");
        exit_code = EXIT_FAILURE;
    } else if(!WIFEXITED(status) || WEXITSTATUS(status) != 0){
        exit_code = EXIT_FAILURE;
    }

    return exit_code;
}
