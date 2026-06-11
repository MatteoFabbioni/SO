/**
Esercizio 2: Linguaggio C: 10 punti
usando "getopt" consentire di scegliere il grado di parallelismo voluto:
    pcp -j 8 file1 file2
deve creare 8 processi, ogni processo copia 1/8 del file.
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
   condiviso del file descriptor, quindi processi diversi (ciascuno con la
   propria copia del fd dopo la fork) possono operare in sicurezza sulla
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

        ssize_t w = pwrite(dst_fd, buf, (size_t)n, start + done);
        if(w == -1){
            perror("pwrite");
            exit(EXIT_FAILURE);
        }

        done += w;
    }
}

int main(int argc, char *argv[]){
    int jobs = 1;
    int opt;

    while((opt = getopt(argc, argv, "j:")) != -1){
        switch(opt){
            case 'j':
                jobs = atoi(optarg);
                if(jobs <= 0){
                    fprintf(stderr, "Error: il grado di parallelismo deve essere positivo\n");
                    return EXIT_FAILURE;
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-j N] file1 file2\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if(optind + 2 != argc){
        fprintf(stderr, "Usage: %s [-j N] file1 file2\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_path = argv[optind];
    const char *dst_path = argv[optind + 1];

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

    /* Crea il file di destinazione della dimensione finale prima di scrivere:
       evita buchi inattesi e garantisce la dimensione corretta indipendentemente
       dall'ordine di completamento dei processi figli */
    if(ftruncate(dst_fd, size) == -1){
        perror("ftruncate");
        close(src_fd);
        close(dst_fd);
        return EXIT_FAILURE;
    }

    /* Se ci sono piu' processi che byte, alcuni processi non avrebbero nulla
       da copiare: limita il parallelismo effettivo alla dimensione del file */
    if((off_t)jobs > size && size > 0)
        jobs = (int)size;

    pid_t *pids = malloc(sizeof(pid_t) * (size_t)jobs);
    if(pids == NULL){
        perror("malloc");
        close(src_fd);
        close(dst_fd);
        return EXIT_FAILURE;
    }

    off_t chunk = size / jobs;
    off_t remainder = size % jobs;
    /* L'ultimo chunk assorbe il resto della divisione intera, cosi' la somma
       delle parti copre esattamente tutto il file */

    off_t offset = 0;
    int children_started = 0;

    for(int i = 0; i < jobs; i++){
        off_t len = chunk + (i == jobs - 1 ? remainder : 0);

        pid_t pid = fork();
        if(pid == -1){
            perror("fork");
            break;
        }

        if(pid == 0){
            copy_range(src_fd, dst_fd, offset, len);
            close(src_fd);
            close(dst_fd);
            free(pids);
            exit(EXIT_SUCCESS);
        }

        pids[i] = pid;
        children_started++;
        offset += len;
    }

    close(src_fd);
    close(dst_fd);

    int exit_code = EXIT_SUCCESS;
    for(int i = 0; i < children_started; i++){
        int status;
        if(waitpid(pids[i], &status, 0) == -1){
            perror("waitpid");
            exit_code = EXIT_FAILURE;
        } else if(!WIFEXITED(status) || WEXITSTATUS(status) != 0){
            exit_code = EXIT_FAILURE;
        }
    }

    free(pids);
    return exit_code;
}
