/**
Esercizio 2: Linguaggio C: 10 punti
Rielaborare l'esercizio precedente per fare in modo che il vettore e le stringhe dei nomi siano allocati
con una unica operazione malloc (in modo da poter liberare lo spazio usato da vreaddir con una unica
operazione free.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

char **vreaddir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    /* Prima passata: conta le entry e somma la lunghezza dei nomi (con terminatore),
       per poter dimensionare un unico blocco che contenga sia il vettore di puntatori
       sia il testo di tutti i nomi. */
    size_t count = 0;
    size_t names_bytes = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        count++;
        names_bytes += strlen(entry->d_name) + 1;
    }

    size_t ptrs_bytes = (count + 1) * sizeof(char *);
    char *block = malloc(ptrs_bytes + names_bytes);
    if (!block) {
        perror("malloc");
        closedir(dir);
        return NULL;
    }

    char **names = (char **) block;
    char *text = block + ptrs_bytes;
    /* Le stringhe vengono scritte subito dopo l'area dei puntatori, nello stesso blocco */

    rewinddir(dir);
    size_t i = 0;
    while ((entry = readdir(dir)) != NULL && i < count) {
        size_t len = strlen(entry->d_name) + 1;
        memcpy(text, entry->d_name, len);
        names[i] = text;
        text += len;
        i++;
    }
    names[count] = NULL;

    closedir(dir);
    return names;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a directory.\n", argv[1]);
        return 1;
    }

    char **names = vreaddir(argv[1]);
    if (!names) {
        fprintf(stderr, "vreaddir failed on %s\n", argv[1]);
        return 1;
    }

    for (size_t i = 0; names[i] != NULL; i++) {
        printf("%s\n", names[i]);
    }

    free(names);
    /* Un'unica free libera vettore e stringhe: sono nello stesso blocco allocato */

    return 0;
}
