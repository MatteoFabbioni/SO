/**
Scrivere un programma sha1update che aggiorna i file creati dal 
programma dell’esercizio 1. Per ogni link simbolico presente nella directory nascosta .sha1index (se c’è) 
possono verificarsi due casi:
•il file è stato cancellato (il link punta a un file inesistente), viene cancellato il link
•il file è stato modificato più recentemente della data di creazione del link, occorre aggiornare il 
link simbolico (si ricalcola la hash e si ricrea  il link simbolico)
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

int main(int argc, char *argv[]) {

    char *dir_path = (argc > 1) ? argv[1] : ".";
    /* Se viene passato un argomento usa quello come directory di lavoro, altrimenti usa "." */

    char sha1_dir[PATH_MAX];
    snprintf(sha1_dir, sizeof(sha1_dir), "%s/.sha1index", dir_path);
    /* Costruisce il path della directory nascosta .sha1index */

    DIR *dir = opendir(sha1_dir);
    /* Apre la directory .sha1index per analizzare i link simbolici presenti */

    if(dir == NULL) {
        if (errno == ENOENT) {
            /* Se la directory non esiste, non c'è nulla da aggiornare */
            return EXIT_SUCCESS;
        }
        perror("opendir .sha1index");
        return EXIT_FAILURE;
    }

    struct dirent *entry;

    while((entry = readdir(dir)) != NULL) {

        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        /* Salta le voci speciali "." e ".." */

        char link_path[PATH_MAX];
        snprintf(link_path, sizeof(link_path), "%s/%s", sha1_dir, entry->d_name);
        /* Costruisce il path completo del link simbolico corrente */

        struct stat link_st, file_st;
        /* Strutture per i metadati del link (link_st) e del file puntato (file_st) */

        if(lstat(link_path, &link_st) == -1) {
            perror("lstat");
            continue;
        }
        /* lstat() legge le info DEL LINK. Se fallisce, passa al prossimo */

        if(!S_ISLNK(link_st.st_mode))
            continue;
        /* Si assicura che la voce in .sha1index sia effettivamente un link simbolico */

        if(stat(link_path, &file_st) == -1) {
            /* stat() "segue" il link e legge le info del FILE PUNTATO */
            
            if(errno == ENOENT) {
                /* CASO 1: stat fallisce con ENOENT (No such file or directory)
                   Il file originale è stato cancellato. Il link è "orfano". */
                if(unlink(link_path) == -1) {
                    perror("unlink orfano");
                }
            } else {
                perror("stat");
            }
            continue; /* Passa al prossimo link */
        }

        /* CASO 2: Il file esiste. Verifichiamo se è stato modificato di recente.
           Confrontiamo il tempo di modifica del file con quello del link. */
        if(file_st.st_mtime > link_st.st_mtime) {

            char link_target[PATH_MAX];
            ssize_t len = readlink(link_path, link_target, sizeof(link_target) - 1);
            /* readlink() legge dove punta il link (es. "../nomefile") */

            if (len == -1) {
                perror("readlink");
                continue;
            }
            link_target[len] = '\0'; /* readlink non aggiunge il terminatore di stringa! */

            /* Costruiamo il path del file originale per passarlo a sha1sum.
               Il target è "../nomefile", quindi estraiamo "nomefile" saltando i primi 3 caratteri */
            char *filename = link_target;
            if (strncmp(link_target, "../", 3) == 0) {
                filename += 3;
            }

            char file_path[PATH_MAX];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, filename);
            /* Path completo del file: dir_path/nomefile */

            /* === RICALCOLO DELL'HASH TRAMITE FORK/PIPE === */
            int pipefd[2];
            if (pipe(pipefd) == -1) { perror("pipe"); continue; }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                close(pipefd[0]); close(pipefd[1]);
                continue;
            }

            if (pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);

                char *args[] = {"sha1sum", file_path, NULL};
                execvp("sha1sum", args);
                perror("execvp");
                exit(EXIT_FAILURE);
            }

            close(pipefd[1]);
            char output[256];
            ssize_t n = read(pipefd[0], output, sizeof(output) - 1);
            close(pipefd[0]);
            wait(NULL);

            if (n >= 40) {
                output[40] = '\0'; /* Tronca all'hash hex di 40 caratteri */

                /* Se l'hash è effettivamente cambiato rispetto al nome del vecchio link */
                if (strcmp(entry->d_name, output) != 0) {
                    
                    char new_link_path[PATH_MAX + 64];
                    snprintf(new_link_path, sizeof(new_link_path), "%s/%.40s", sha1_dir, output);
                    /* Crea il path per il nuovo link */

                    if(symlink(link_target, new_link_path) == -1 && errno != EEXIST) {
                        perror("symlink nuovo");
                    } else {
                        /* Se il nuovo link è stato creato con successo, elimina il vecchio */
                        unlink(link_path);
                    }
                } else {
                    /* Il file è stato toccato (mtime aggiornato) ma il contenuto (hash) è identico.
                       Ricreiamo semplicemente il link per aggiornare la sua data st_mtime. */
                    unlink(link_path);
                    if (symlink(link_target, link_path) == -1)
                        perror("symlink ricreazione");
                }
            }
        }
    }

    closedir(dir);
    return EXIT_SUCCESS;
}