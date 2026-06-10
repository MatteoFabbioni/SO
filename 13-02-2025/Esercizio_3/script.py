#!/usr/bin/env python3

"""
Scrivere un programma Python o uno script bash che presi come parametri i pathname di un file f e
di una directory d stampi l'elenco dei file che hanno la stessa ampiezza (numero di byte) di f ma non
sono link fisici di f presenti nel sottoalbero del file system generato dalla directory d.
"""

import os
import sys

def main():
    if len(sys.argv) != 3:
        print("Uso: ./script.py <file> <directory>", file=sys.stderr)
        sys.exit(1)

    target = sys.argv[1]

    # os.stat segue i symlink: otteniamo inode, device e dimensione del file reale
    try:
        targetStat = os.stat(target)
    except OSError as e:
        print(f"Errore sul file target: {e}", file=sys.stderr)
        sys.exit(1)

    # Il programma accetta solo file regolari come target
    if not os.path.isfile(target):
        print(f"Errore: {target} non è un file regolare", file=sys.stderr)
        sys.exit(1)

    # os.walk visita ricorsivamente il sottoalbero di sys.argv[2].
    # Per default non segue symlink a directory, evitando loop su cicli nel filesystem.
    # dirnames è ignorato (_) perché non ci serve filtrare la discesa.
    for dirpath, _, filenames in os.walk(sys.argv[2]):
        for name in filenames:
            full = os.path.join(dirpath, name)

            # lstat non segue symlink: legge i metadati dell'entry stessa.
            # Questo evita crash su symlink pendenti (target inesistente).
            try:
                fullStat = os.lstat(full)
            except OSError:
                continue

            # Un hard link di f condivide sia inode che device.
            # Confrontare solo l'inode non basta: filesystem diversi possono
            # avere inode uguali per file non correlati.
            if fullStat.st_ino == targetStat.st_ino and fullStat.st_dev == targetStat.st_dev:
                continue

            # Stampa i file con la stessa dimensione di f che non sono suoi hard link
            if fullStat.st_size == targetStat.st_size:
                print(full)

if __name__ == "__main__":
    main()
