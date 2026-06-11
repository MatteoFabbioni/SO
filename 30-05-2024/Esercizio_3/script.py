#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10  punti
Scrivere un programma o uno script  lscmd che consenta ad un utente di poter avere l'elenco di tutti i
pid dei suoi processi in esecuzione raggruppati per pathname del programma eseguito..
Es:
     $ lscmd
     /usr/bin/bash 2021 2044
     /usr/bin/xterm 2010
,,,
"""

import os
import sys


def main():
    uid = os.getuid()
    grouped = {}

    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        pid = entry
        exe_path = f"/proc/{pid}/exe"

        try:
            # lstat: la entry e' un symlink a se stesso, non vogliamo seguirlo
            # qui solo per controllare il proprietario del processo
            st = os.lstat(f"/proc/{pid}")
        except OSError:
            continue

        if st.st_uid != uid:
            continue

        try:
            target = os.readlink(exe_path)
        except OSError:
            # processo terminato nel frattempo, oppure exe non accessibile
            # (es. processo del kernel o di un altro namespace)
            continue

        grouped.setdefault(target, []).append(pid)

    for path in sorted(grouped):
        pids = sorted(grouped[path], key=int)
        print(path, " ".join(pids))


if __name__ == "__main__":
    main()
