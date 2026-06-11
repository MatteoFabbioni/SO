/**
 * Esercizio 2: Linguaggio C: 10 punti
 * Estendere l'esercizio 1 per fare in modo che se prima del timeout il programma termina con un errore,
 * al termine del timeout il programma venga riattivato.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

/* Lancia argv[2..] e ne ritorna il pidfd; pid_out riceve il pid del figlio. */
static int spawn(char *argv[], pid_t *pid_out) {
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

    *pid_out = pid;
    return pfd;
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

    pid_t pid;
    int pfd = spawn(argv, &pid);
    int status = 0;

    /* Il programma puo' essere rilanciato una volta, alla scadenza del timeout,
     * solo se la sua precedente esecuzione e' terminata con un errore prima del timeout stesso. */
    int restarted = 0;

    for (;;) {
        struct pollfd pfds[2];
        pfds[0].fd = pfd;
        pfds[0].events = POLLIN;
        pfds[1].fd = tfd;
        pfds[1].events = POLLIN;

        if (poll(pfds, 2, -1) < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (pfds[0].revents & POLLIN) {
            waitpid(pid, &status, 0);
            close(pfd);

            if (!restarted && WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                /* Errore prima del timeout: si attende la scadenza del timer
                 * (gia' impostato all'avvio) e poi si riattiva il programma.
                 * Il timerfd resta "readable" finche' non si legge il contatore
                 * delle scadenze: va consumato per non farlo risultare ancora
                 * pronto nella prossima poll(), altrimenti il programma riavviato
                 * verrebbe ucciso immediatamente. */
                struct pollfd tpfd = { .fd = tfd, .events = POLLIN };
                uint64_t expirations;
                poll(&tpfd, 1, -1);
                read(tfd, &expirations, sizeof(expirations));

                restarted = 1;
                pfd = spawn(argv, &pid);
                continue;
            }
            break;
        } else {
            /* e' scattato il timer: il programma in corso ha superato la durata massima */
            if (syscall(SYS_pidfd_send_signal, pfd, SIGKILL, NULL, 0) < 0) {
                perror("pidfd_send_signal");
            }
            waitpid(pid, &status, 0);
            close(pfd);
            fprintf(stderr, "timeout: il programma ha superato i %ld ms ed e' stato terminato\n", ms);
            break;
        }
    }

    close(tfd);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return EXIT_FAILURE;
}
