/**
Chiamiamo file path ricorsivo un file che contiene
una sequenza di byte identica al proprio pathname completo. Ad esempio un file myfile nella
directory /tmp/mydir è path ricorsivo se contiene la sequenza /tmp/mydir/myfile. Il candidato scriva il
programma search_prec (senza parametri) che deve scorrere la directory corrente e elencare tutti i file
path ricorsivi.
 */

#define _DEFAULT_SOURCE          // abilita le estensioni POSIX/BSD su Linux
#include<stdio.h>                // printf, fprintf, fopen, fclose, fread, perror
#include<stdlib.h>               // malloc, free, exit, EXIT_FAILURE
#include<dirent.h>               // DIR, struct dirent, opendir, readdir, closedir
#include<string.h>               // strcmp, strstr, snprintf
#include<sys/stat.h>             // struct stat, stat(), S_ISREG()
#include<limits.h>               // PATH_MAX: lunghezza massima di un pathname
#include<unistd.h>               // getcwd()

int main(int argc, char *argv[]){
    if(argc != 1){                                      // il programma non accetta argomenti
        fprintf(stderr, "Usage: %s\n", argv[0]);        // stampa l'uso su stderr
        exit(EXIT_FAILURE);                             // termina con errore
    }

    DIR *dir = opendir(".");    // apre la directory corrente e restituisce un handle
    struct stat st;             // conterrà i metadati (dimensione, tipo) di ogni file
    FILE *fp;                   // puntatore al file aperto per la lettura del contenuto
    char cwd[PATH_MAX];         // buffer per il path assoluto della directory corrente
    char full_path[PATH_MAX];   // buffer per il path completo di ogni entry

    if(dir == NULL){            // opendir fallisce se la directory non è accessibile
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;                // puntatore all'entry corrente della directory
    getcwd(cwd, sizeof(cwd));           // riempie cwd con il path assoluto (es. /tmp/mydir)

    while((entry = readdir(dir)) != NULL){  // itera su tutte le entry della directory
        // salta le entry speciali "." (dir corrente) e ".." (dir padre)
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        // costruisce il pathname completo: es. /tmp/mydir/myfile
        snprintf(full_path, sizeof(full_path), "%s/%s", cwd, entry->d_name);

        if(stat(full_path, &st) == -1){  // legge i metadati del file
            perror("stat");
            continue;                    // se fallisce, passa all'entry successiva
        }

        if(S_ISREG(st.st_mode)){         // processa solo file regolari (no dir, no symlink)
            fp = fopen(full_path, "r");  // apre il file in lettura
            if(fp == NULL){
                perror("fopen");
                continue;               // se fallisce, passa all'entry successiva
            }

            char *buffer = malloc(st.st_size + 1);  // alloca buffer grande quanto il file + '\0'
            if(buffer == NULL){
                perror("malloc");
                fclose(fp);
                continue;
            }

            // legge l'intero contenuto del file nel buffer
            if(fread(buffer, 1, st.st_size, fp) != st.st_size){
                perror("fread");
                free(buffer);
                fclose(fp);
                continue;               // lettura incompleta: passa all'entry successiva
            }

            buffer[st.st_size] = '\0';  // termina la stringa per usare strstr

            // se il contenuto del file contiene il suo pathname completo: è path-ricorsivo
            if(strstr(buffer, full_path) != NULL){
                printf("%s\n", entry->d_name);  // stampa il nome del file
            }

            free(buffer);   // libera il buffer allocato
            fclose(fp);     // chiude il file
        }
    }

    closedir(dir);  // chiude l'handle della directory
    return 0;
}
