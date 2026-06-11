#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash che esaminando i dati in /proc/*/status fornisca in
output una tabella che indichi per ogni processo di proprietà dell'utente, il nome dell'eseguibile e
l'attuale occupazione di memoria (campo vmSize).
"""

import os
import sys

def main():
    my_uid = os.getuid()

    print(f"{'PID':<8}{'NOME':<20}{'VMSIZE':<15}")

    for pid in sorted(os.listdir("/proc"), key=lambda s: (not s.isdigit(), s)):
        if not pid.isdigit():
            continue

        status_path = f"/proc/{pid}/status"
        try:
            with open(status_path) as f:
                lines = f.readlines()
        except (FileNotFoundError, ProcessLookupError):
            # il processo può terminare tra la lettura di /proc e l'apertura del suo status
            continue
        except PermissionError:
            continue

        name = None
        vmsize = None
        uid = None
        for line in lines:
            if line.startswith("Name:"):
                name = line.split(maxsplit=1)[1].strip()
            elif line.startswith("VmSize:"):
                vmsize = line.split(maxsplit=1)[1].strip()
            elif line.startswith("Uid:"):
                # il primo dei quattro campi (real, effective, saved, fs) è l'uid reale
                uid = int(line.split()[1])

        if uid != my_uid:
            continue

        # i processi senza memoria virtuale propria (es. kernel thread) non hanno VmSize
        if vmsize is None:
            vmsize = "n/d"

        print(f"{pid:<8}{name:<20}{vmsize:<15}")


if __name__ == "__main__":
    main()
