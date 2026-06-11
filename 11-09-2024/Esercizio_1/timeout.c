/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * Scrivere un programma timeout che esegua un programma e lo termini se supera una durata
 * massima prefissata. timeout ha almeno due argomenti: il primo è la durata massima in millisecondi, i
 * parametri dal secondo in poi sono il programma da lanciare coi rispettivi argomenti.
 * Es:
 * timeout 5000 sleep 2
 * temina in due secondi (sleep termina in tempo).
 * timeout 3000 sleep 5
 * passati tre secondi il programma sleep viene terminato.
 * Tmeout deve essere scritto usando le system call poll, pidfd_open, timerfd*.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/timerfd.h>

/* pidfd_open non ha un wrapper in glibc per le versioni meno recenti:
 * si invoca direttamente la syscall. */
static int pidfd_open(pid_t pid, unsigned int flags) {
    return syscall(SYS_pidfd_open, pid, flags);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <millisecondi> <programma> [argomenti...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    long ms = atol(argv[1]);
    if (ms <= 0) {
        fprintf(stderr, "Durata non valida: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execvp(argv[2], &argv[2]);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    int pfd = pidfd_open(pid, 0);
    if (pfd < 0) {
        perror("pidfd_open");
        exit(EXIT_FAILURE);
    }

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd < 0) {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    struct itimerspec its;
    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = (ms % 1000) * 1000000L;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    if (timerfd_settime(tfd, 0, &its, NULL) < 0) {
        perror("timerfd_settime");
        exit(EXIT_FAILURE);
    }

    struct pollfd pfds[2];
    pfds[0].fd = pfd;
    pfds[0].events = POLLIN;
    pfds[1].fd = tfd;
    pfds[1].events = POLLIN;

    if (poll(pfds, 2, -1) < 0) {
        perror("poll");
        exit(EXIT_FAILURE);
    }

    int status;
    if (pfds[0].revents & POLLIN) {
        /* il processo figlio e' terminato prima del timeout */
        waitpid(pid, &status, 0);
    } else {
        /* e' scattato il timer: il processo ha superato la durata massima */
        if (syscall(SYS_pidfd_send_signal, pfd, SIGKILL, NULL, 0) < 0) {
            perror("pidfd_send_signal");
        }
        waitpid(pid, &status, 0);
        fprintf(stderr, "timeout: il programma ha superato i %ld ms ed e' stato terminato\n", ms);
    }

    close(pfd);
    close(tfd);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return EXIT_FAILURE;
}
