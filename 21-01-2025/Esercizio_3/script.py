#!/usr/bin/env python3

"""
Lo script o il programma Python deve fornire una lista dei file
all'interno di un sottoalbero del file system ordinati secondo la "profondità" a partire da quelli più
profondi, per ultimi quelli della directory radice. I nodi allo stesso livello devono essere ordinati in
ordine crescente del nome del file.
"""

import os
import sys

def main():
    # Lo script accetta al massimo un argomento (la directory radice)
    if len(sys.argv) > 2:
        print("Uso: ./script.py [<directory>]", file=sys.stderr)
        sys.exit(1)

    # Se l'argomento è fornito lo usiamo, altrimenti partiamo dalla directory corrente
    if len(sys.argv) == 2:
        path_input = sys.argv[1]
    else:
        path_input = "."

    if not os.path.isdir(path_input):
        print(f"Errore: la directory '{path_input}' non esiste.", file=sys.stderr)
        sys.exit(1)

    file_trovati = []

    # Convertiamo in percorso assoluto per calcolare la profondità in modo coerente
    dir_radice = os.path.abspath(path_input)

    for dirpath, _, filenames in os.walk(dir_radice):
        # Profondità relativa alla radice: contiamo i separatori nel percorso
        # che eccede la radice stessa (es. radice/a/b → profondità 2)
        depth = dirpath[len(dir_radice):].count(os.sep)
        for name in sorted(filenames):
            full = os.path.join(dirpath, name)
            # Ogni entry è una tupla (profondità, nome, percorso_completo)
            file_trovati.append((depth, name, full))

    # Ordinamento per due chiavi:
    # 1. -depth (negativo): i file più profondi vengono prima (ordine decrescente di profondità)
    # 2. name: a parità di profondità, ordine alfabetico crescente del nome
    file_trovati.sort(key=lambda x: (-x[0], x[1]))

    for _, _, full in file_trovati:
        print(full)


if __name__ == "__main__":
    main()
