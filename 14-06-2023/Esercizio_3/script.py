#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash che, passata una directory come parametro, cancelli
nel sottoalbero generato dalla directory passata come parametro tutti i link simbolici relativi (e non
cancelli quelli assoluti)
lrwxrwxrwx 1 renzo renzo 13 Jun 11 17:03 hostname1 -> /etc/hostname
lrwxrwxrwx 1 renzo renzo 15 Jun 11 17:04 hostname2 -> ../etc/hostname
il primo va mantenuto e il secondo cancellato
"""

import os
import sys


def cancella_link_relativi(directory):
    for root, dirs, files in os.walk(directory, followlinks=False):
        # followlinks=False: le directory puntate da symlink non vanno attraversate,
        # altrimenti os.walk potrebbe finire in loop su un ciclo del filesystem
        for name in dirs + files:
            full_path = os.path.join(root, name)
            if os.path.islink(full_path):
                target = os.readlink(full_path)
                # un link e' assoluto se il target inizia con "/", altrimenti e' relativo
                if not os.path.isabs(target):
                    try:
                        os.remove(full_path)
                    except OSError as e:
                        print(f"Errore nella cancellazione di {full_path}: {e}", file=sys.stderr)


def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    directory = sys.argv[1]

    if not os.path.isdir(directory):
        print(f"{directory} non e' una directory valida.", file=sys.stderr)
        sys.exit(1)

    cancella_link_relativi(directory)


if __name__ == "__main__":
    main()
