/**
Esercizio 2: Linguaggio C: 10 punti
Estendere l'esercizio 1. Il nuovo programma autolancia deve riconoscere se il primo parametro è una
libreria dinamica o un eseguibile gestendo entrambi i casi:
		gcc -o hw hw.c
		$ ./autolancia hw.so 1 2 3 4
		hello world: hw.so 1 2 3 4
		$ ./autolancia hw 1 2 3 4
		hello world: hw.so 1 2 3 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <elf.h>
#include <limits.h>

typedef int (*main_func_t)(int, char **);

/* Su Linux sia le librerie condivise sia gli eseguibili PIE hanno e_type
   ET_DYN: il discriminante affidabile e' la presenza del program header
   PT_INTERP, che esiste solo negli eseguibili (indica il dynamic linker). */
static int is_executable(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    Elf64_Ehdr ehdr;
    if (fread(&ehdr, sizeof(ehdr), 1, f) != 1) {
        fprintf(stderr, "Error reading ELF header from %s\n", path);
        fclose(f);
        exit(1);
    }

    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "%s: not an ELF file\n", path);
        fclose(f);
        exit(1);
    }

    if (ehdr.e_type == ET_EXEC) {
        fclose(f);
        return 1;
    }
    if (ehdr.e_type != ET_DYN) {
        fprintf(stderr, "%s: unsupported ELF type\n", path);
        fclose(f);
        exit(1);
    }

    if (fseek(f, (long) ehdr.e_phoff, SEEK_SET) == -1) {
        perror("fseek");
        fclose(f);
        exit(1);
    }

    int found_interp = 0;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        Elf64_Phdr phdr;
        if (fread(&phdr, sizeof(phdr), 1, f) != 1) {
            fprintf(stderr, "Error reading program header from %s\n", path);
            fclose(f);
            exit(1);
        }
        if (phdr.p_type == PT_INTERP) {
            found_interp = 1;
            break;
        }
    }

    fclose(f);
    return found_interp;
}

static int lancia_libreria(const char *path, int argc, char *argv[]) {
    char abspath[PATH_MAX];
    if (realpath(path, abspath) == NULL) {
        perror("realpath");
        return 1;
    }

    void *handle = dlopen(abspath, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }

    dlerror();
    main_func_t hw_main = (main_func_t) dlsym(handle, "main");
    char *err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "dlsym: %s\n", err);
        dlclose(handle);
        return 1;
    }

    int ret = hw_main(argc, argv);
    dlclose(handle);
    return ret;
}

static int lancia_eseguibile(char *argv[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execv(argv[0], argv);
        perror("execv");
        exit(1);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hw.so|hw> [args...]\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a regular file\n", argv[1]);
        return 1;
    }

    if (is_executable(argv[1])) {
        return lancia_eseguibile(argv + 1);
    } else {
        return lancia_libreria(argv[1], argc - 1, argv + 1);
    }
}
