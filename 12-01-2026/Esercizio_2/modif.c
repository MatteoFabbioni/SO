/**
Scrivere il comando modif= che opera come modifcmp ma invece di controllare i file modificati più di 
recente, elenca i pathname dei file che hanno il tempo di modifica uguale a quella del file indicato nel 
primo parametro che non sia un link fisico o logico di tale file.   Occorre mostrare anche come 
costruire un esempio per testare il programma.
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX e GNU: necessario per rendere visibile PATH_MAX da <limits.h> */

#include<stdio.h>
/* Fornisce printf() e perror() per stampare output e messaggi di errore */

#include<stdlib.h>
/* Libreria standard C: non usata direttamente ma buona pratica includerla */

#include<sys/stat.h>
/* Fornisce stat(), struct stat e le macro S_ISREG(), S_ISDIR() per ispezionare i file */

#include<dirent.h>
/* Fornisce opendir(), readdir(), closedir() e struct dirent per scorrere le directory */

#include<string.h>
/* Fornisce strcmp() per confrontare stringhe (usato per saltare "." e "..") */

#include<unistd.h>
/* Fornisce costanti e tipi POSIX standard */

#include<time.h>
/* Fornisce il tipo time_t usato per rappresentare i timestamp di modifica */

#include<limits.h>
/* Fornisce PATH_MAX: la lunghezza massima di un pathname sul sistema */

void walk(const char *path, time_t ref_time, ino_t ref_ino, dev_t ref_dev){
/* Funzione ricorsiva che scorre il sottoalbero di 'path' e stampa i file con mtime
   uguale a ref_time, escludendo hard link e symbolic link del file di riferimento.
   ref_ino e ref_dev identificano univocamente il file di riferimento (inode + device) */

    DIR *dir = opendir(path);
    /* Apre la directory 'path' e restituisce un puntatore DIR per iterarla */

    if(!dir){
        /* Se opendir fallisce (es. permessi negati) dir è NULL */

        perror("Error opening directory");
        /* Stampa il messaggio di errore di sistema corrispondente all'errno corrente */

        return;
        /* Esce dalla funzione senza crashare: gli altri rami dell'albero continuano */
    }

    struct dirent *entry;
    /* Struttura che conterrà le informazioni di ogni voce letta dalla directory */

    while((entry = readdir(dir)) != NULL){
        /* readdir() restituisce la prossima voce della directory, NULL quando finisce */

        if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")){
            /* Salta le voci speciali "." (directory corrente) e ".." (directory padre)
               per evitare loop infiniti nella ricorsione */

            continue;
            /* Passa alla prossima iterazione del while */
        }

        char full_path[PATH_MAX];
        /* Buffer che conterrà il pathname completo della voce corrente */

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        /* Costruisce il pathname completo concatenando la directory corrente e il nome della voce.
           snprintf limita la scrittura a sizeof(full_path) byte per evitare buffer overflow */

        struct stat st;
        /* Struttura che conterrà le informazioni (tipo, mtime, inode, ecc.) della voce corrente */

        if(lstat(full_path, &st) == -1) continue;
        /* lstat() riempie 'st' con i metadati di full_path SENZA seguire i symlink.
           Usando lstat possiamo rilevare i symlink tramite S_ISLNK.
           Se fallisce restituisce -1: si salta la voce e si continua */

        if(S_ISDIR(st.st_mode)){
            /* S_ISDIR controlla se la voce è una directory */

            walk(full_path, ref_time, ref_ino, ref_dev);
            /* Chiamata ricorsiva: scende nel sottoalbero della sottodirectory trovata */

        }else{
            /* La voce è un file regolare, un symlink o altro tipo non-directory */

            if(S_ISLNK(st.st_mode)){
                /* S_ISLNK rileva i symbolic link (possibile grazie a lstat).
                   Un symlink al file di riferimento va escluso per requisito */

                continue;
                /* Salta il symlink */
            }

            if(st.st_ino == ref_ino && st.st_dev == ref_dev){
                /* Due file con stesso inode (st_ino) e stesso device (st_dev) sono hard link
                   dello stesso file fisico. Vanno esclusi per requisito */

                continue;
                /* Salta l'hard link */
            }

            if(st.st_mtime == ref_time){
                /* Se il timestamp di modifica è uguale a quello del file di riferimento,
                   e non è né un symlink né un hard link, il file soddisfa i criteri */

                printf("%s\n", full_path);
                /* Stampa il pathname completo del file trovato */
            }
        }
    }

    closedir(dir);
    /* Chiude il descrittore della directory liberando le risorse di sistema */
}

int main(int argc, char *argv[]){
/* Punto di ingresso del programma.
   argc = numero di argomenti (incluso il nome del programma)
   argv = array di stringhe con gli argomenti */

    if(argc == 1){
        /* Nessun argomento passato: solo il nome del programma */

        printf("Usage: %s <file1> [<file2>|<directory>]\n", argv[0]);
        /* Stampa le istruzioni d'uso. argv[0] è il nome dell'eseguibile */

        return -1;
        /* Termina con codice di errore */

    }else if(argc == 2){
        /* Un solo argomento: modalità "scansiona la directory corrente" */

        struct stat st;
        /* Struttura per i metadati del file passato come argomento */

        if(lstat(argv[1], &st) == -1){
            /* Ottiene i metadati di argv[1]; se fallisce il file non esiste o non è accessibile */

            perror("Error stat file");
            /* Stampa l'errore di sistema */

            return -1;
            /* Termina con codice di errore */
        }

        if(!S_ISREG(st.st_mode)){
            /* Controlla che argv[1] sia un file regolare (non una directory, symlink, ecc.) */

            printf("Error: %s is not a regular file.\n", argv[1]);
            /* Segnala che il primo argomento deve essere un file regolare */

            return -1;
            /* Termina con codice di errore */
        }

        walk(".", st.st_mtime, st.st_ino, st.st_dev);
        /* Scansiona ricorsivamente la directory corrente (".") cercando file con lo stesso
           timestamp di argv[1], escludendo i suoi hard link e symbolic link */

    }else if(argc == 3){
        /* Due argomenti: modalità "file vs file" oppure "file vs directory" */

        struct stat st1, st2;
        /* Due strutture per i metadati dei due argomenti */

        if(lstat(argv[1], &st1) == -1){
            /* lstat() ottiene i metadati di argv[1] senza seguire symlink,
               coerente con il caso argc==2 */

            perror("Error stat file1");
            return -1;
        }

        if(!S_ISREG(st1.st_mode)){
            /* Il primo argomento deve sempre essere un file regolare */

            printf("Error: %s is not a regular file.\n", argv[1]);
            return -1;
        }

        if(stat(argv[2], &st2) == -1){
            /* Ottiene i metadati del secondo argomento (file o directory) */

            perror("Error stat file2/directory");
            return -1;
        }

        if(S_ISREG(st2.st_mode)){
            /* Il secondo argomento è un file regolare: confronto diretto tra i due file */

            if(st2.st_ino == st1.st_ino && st2.st_dev == st1.st_dev){
                /* Stesso inode e device: argv[2] è un hard link di argv[1], va escluso */

            }else if(st2.st_mtime == st1.st_mtime){
                /* Stesso timestamp e non è un hard link: lo stampa */

                printf("%s\n", argv[2]);
            }

        }else if(S_ISDIR(st2.st_mode)){
            /* Il secondo argomento è una directory: scansiona il suo sottoalbero */

            walk(argv[2], st1.st_mtime, st1.st_ino, st1.st_dev);
            /* Cerca nel sottoalbero di argv[2] i file con lo stesso timestamp di argv[1],
               escludendo hard link e symbolic link */

        }else{
            /* Il secondo argomento non è né un file regolare né una directory */

            printf("Error: %s is neither a regular file nor a directory.\n", argv[2]);
            return -1;
        }

    }else{
        /* Più di 3 argomenti: uso non valido */

        printf("Usage: %s <file1> [<file2>|<directory>]\n", argv[0]);
        return -1;
    }

    return 0;
    /* Termina il programma con successo (codice 0) */
}
