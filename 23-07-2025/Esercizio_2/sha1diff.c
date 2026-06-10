/**
Scrivere un programma sha1diff che usando gli stessi parametri 
passati precedentemente al programma sha1dir dell'esercizio 1 e mette in output l'elenco dei file che 
sono stati modificati (la hash sha1 non corrisponde).
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX/GNU: necessario per mkdir(), opendir(), readdir() */

#include <stdio.h>
/* Fornisce fprintf(), perror(), fopen(), fclose(), fprintf() */

#include <stdlib.h>
/* Fornisce EXIT_SUCCESS, EXIT_FAILURE, exit() */

#include <string.h>
/* Fornisce strcmp() per confrontare i nomi delle voci di directory */

#include <sys/stat.h>
/* Fornisce stat(), mkdir(), struct stat e le macro S_ISDIR(), S_ISREG() */

#include <dirent.h>
/* Fornisce opendir(), readdir(), closedir() e struct dirent */

#include <unistd.h>
/* Fornisce close(), dup2(), read(), execvp() */

#include <errno.h>
/* Fornisce errno e le costanti di errore */

#include <limits.h>
/* Fornisce PATH_MAX: lunghezza massima di un pathname sul sistema */

#include <sys/types.h>
/* Fornisce pid_t: tipo per i PID dei processi */

#include <sys/wait.h>
/* Fornisce wait(): attende la terminazione di un processo figlio */

void copy_directory(const char *src, const char *dst){
    /* Funzione ricorsiva: replica la struttura di 'src' in 'dst'.
       I file regolari vengono sostituiti con file contenenti il loro hash SHA1 */

    DIR *dir = opendir(src);
    /* Apre la directory sorgente per poterne leggere le voci */

    if(dir == NULL){
        perror("Error opening source directory");
        /* Stampa l'errore e termina la funzione senza fare nulla */
        return;
    }

    struct dirent *entry;
    /* Puntatore alla struttura che conterrà le informazioni di ogni voce letta */

    while((entry = readdir(dir)) != NULL){
        /* readdir() restituisce la prossima voce della directory, NULL quando finisce */

        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        /* Salta "." (directory corrente) e ".." (directory padre) per evitare loop infiniti */

        char src_path[PATH_MAX];
        /* Buffer per il path completo della voce nella directory sorgente */

        char dst_path[PATH_MAX];
        /* Buffer per il path completo della voce nella directory destinazione */

        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        /* Costruisce il path sorgente completo: es. "/tmp/test_src/file1.txt" */

        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
        /* Costruisce il path destinazione completo: es. "/tmp/test_dst/file1.txt" */

        struct stat st;
        /* Struttura che conterrà i metadati (tipo, permessi, ecc.) della voce corrente */

        if(stat(src_path, &st) != 0){
            perror("Error getting file status");
            continue;
            /* Se stat() fallisce salta questa voce e passa alla successiva */
        }

        if(S_ISDIR(st.st_mode)){

            copy_directory(src_path, dst_path);
            /* Chiamata ricorsiva: replica il contenuto della sottodirectory */

        } else if(S_ISREG(st.st_mode)){
            /* La voce è un file regolare: calcola il suo SHA1 e lo scrive nella destinazione */

            int pipefd[2];
            /* Array di due descrittori: pipefd[0]=lettura, pipefd[1]=scrittura */

            if(pipe(pipefd) == -1){ perror("pipe"); continue; }
            /* Crea la pipe; se fallisce salta il file */

            pid_t pid = fork();
            /* Duplica il processo: restituisce 0 al figlio, il PID del figlio al padre, -1 in caso di errore */

            if(pid == -1){
                perror("fork");
                close(pipefd[0]); close(pipefd[1]);
                /* In caso di errore chiude entrambe le estremità della pipe */
                continue;
            }

            if(pid == 0){
                /* Blocco eseguito solo dal processo figlio */

                close(pipefd[0]);
                /* Il figlio non legge dalla pipe: chiude l'estremità di lettura */

                dup2(pipefd[1], STDOUT_FILENO);
                /* Reindirizza lo stdout del figlio verso la pipe:
                   tutto ciò che sha1sum stampa finirà nella pipe */

                close(pipefd[1]);
                /* Il descrittore originale della pipe non serve più: è già duplicato su STDOUT_FILENO */

                char *args[] = {"sha1sum", src_path, NULL};
                /* Array di argomenti per execvp: nome programma, file da hashare, terminatore NULL */

                execvp("sha1sum", args);
                /* Sostituisce il processo figlio con sha1sum; se ha successo non ritorna mai */

                perror("execvp");
                exit(EXIT_FAILURE);
                /* Raggiunto solo se execvp fallisce (es. sha1sum non trovato nel PATH) */
            }

            /* Da qui in poi: solo il processo padre */

            close(pipefd[1]);
            /* Il padre non scrive nella pipe: chiude l'estremità di scrittura.
               Necessario affinché read() riceva EOF quando sha1sum termina */

            char output[256];
            /* Buffer per leggere l'output di sha1sum (formato: "hash40char  nomefile\n") */

            ssize_t n = read(pipefd[0], output, sizeof(output) - 1);
            /* Legge l'output di sha1sum dalla pipe */

            close(pipefd[0]);
            /* Chiude l'estremità di lettura dopo aver letto */

            wait(NULL);
            /* Attende la terminazione del figlio per evitare processi zombie.
               NULL indica che non ci interessa il codice di uscita */

            if(n < 40){ continue; }
            /* Se l'output è troppo corto c'è stato un errore: salta il file */

            output[40] = '\0';
            /* sha1sum stampa "hash  nomefile": tronca dopo i 40 caratteri dell'hash hex */

            FILE *dst_file = fopen(dst_path, "r");
            /* Crea il file di destinazione in scrittura testuale */

            if(dst_file == NULL){
                perror("fopen");
                continue;
                /* Se fopen fallisce salta questo file */
            }

            char saved_hash[41];
            /* Buffer per leggere l'hash salvato in destinazione (40 caratteri + terminatore) */

            fgets(saved_hash, sizeof(saved_hash), dst_file);
            /* Legge l'hash salvato nel file di destinazione */

            fclose(dst_file);
            /* Chiude il file di destinazione: flush e rilascio delle risorse */

            saved_hash[40] = '\0';
            /* Assicura che saved_hash sia una stringa terminata correttamente */

            if(strcmp(output, saved_hash) != 0){
                /* Se l'hash calcolato non corrisponde a quello salvato, il file è stato modificato */

                printf("%s\n", src_path);
                /* Stampa il path del file modificato */
            }
        }
    }

    closedir(dir);
    /* Chiude il descrittore della directory sorgente liberando le risorse di sistema */
}

int main(int argc, char *argv[]){

    if(argc != 3){
        fprintf(stderr, "Usage: %s <source_directory> <destination_directory>\n", argv[0]);
        return EXIT_FAILURE;
        /* Il programma richiede esattamente due argomenti: sorgente e destinazione */
    }

    char *source_dir = argv[1];
    /* Primo argomento: directory sorgente da cui leggere */

    char *dest_dir = argv[2];
    /* Secondo argomento: directory destinazione da creare (deve essere inesistente) */

    struct stat st;
    /* Struttura per verificare l'esistenza e il tipo dei path forniti */

    if(stat(source_dir, &st) != 0 || !S_ISDIR(st.st_mode)){
        fprintf(stderr, "Error: Source directory '%s' does not exist or is not a directory.\n", source_dir);
        return EXIT_FAILURE;
        /* Verifica che la sorgente esista e sia una directory */
    }

    if(stat(dest_dir, &st) != 0 || !S_ISDIR(st.st_mode)){
        fprintf(stderr, "Error: Destination directory '%s' does not exist or is not a directory.\n", dest_dir);
        return EXIT_FAILURE;
        /* Verifica che la destinazione esista e sia una directory */
    }

    copy_directory(source_dir, dest_dir);
    /* Avvia la copia ricorsiva dell'albero di directory */

    return 0;
    /* Termina il programma con codice di successo */
}