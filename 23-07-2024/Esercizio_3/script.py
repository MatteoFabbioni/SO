#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere uno script che prende in input da linea di comando il nome di due directory ed elimina (da
entrambe le directory) tutti i file che hanno la stessa hash sha1. Più precisamente se c'è un file nella
prima directory e uno nella seconda che hanno la stessa hash sha1 tutti i file che hanno la stessa hash
sha1 presenti nelle due directory vanno cancellati.
(Se esistono file con la stessa hash sha1 ma solo in una delle due directory, non sono da cancellare.)
"""

import sys
import os
import hashlib


def sha1_of_file(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def hashes_in_dir(dir_path):
    # mappa hash -> lista di path: serve per poter cancellare anche più file
    # con la stessa hash nella stessa directory, non solo il primo trovato
    result = {}
    for name in os.listdir(dir_path):
        full_path = os.path.join(dir_path, name)
        if os.path.isfile(full_path) and not os.path.islink(full_path):
            digest = sha1_of_file(full_path)
            result.setdefault(digest, []).append(full_path)
    return result


def main():
    if len(sys.argv) != 3:
        print(f"Uso: {sys.argv[0]} <directory1> <directory2>", file=sys.stderr)
        sys.exit(1)

    dir1, dir2 = sys.argv[1], sys.argv[2]

    if not os.path.isdir(dir1):
        print(f"Errore: {dir1} non è una directory", file=sys.stderr)
        sys.exit(1)
    if not os.path.isdir(dir2):
        print(f"Errore: {dir2} non è una directory", file=sys.stderr)
        sys.exit(1)

    hashes1 = hashes_in_dir(dir1)
    hashes2 = hashes_in_dir(dir2)

    common = set(hashes1) & set(hashes2)

    for digest in common:
        for path in hashes1[digest] + hashes2[digest]:
            os.remove(path)


if __name__ == "__main__":
    main()
