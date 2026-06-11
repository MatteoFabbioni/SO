#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: 10 punti
Sia data una directory che contiene file di testo.
Scopo dell'esercizio e' di scrivere un programma Python o uno script bash chiamato ccpl che conti i
caratteri delle corrispondenti righe di testo di tutti i file della directory, si vuole cioe' sapere il numero
totale di caratteri presenti nelle prime righe di tutti i file, nelle seconde linee, ecc.
$ ccpl mydir
1 234
2 21
3 333
…..
l'ouput significa che se contiamo tutti i caratteri contenuti nella prima riga di tutti i file in mydir
otteniamo 234 (mydir/file1 puo' avere 40 caratteri nella prima riga, mydir/file2 ne puo' avere 20, ecc...
procedendo per tutti i file di mydir la somma fa 234).
"""

import os
import sys


def main():
    if len(sys.argv) != 2:                            # ccpl richiede esattamente un argomento: la directory
        print(f"Uso: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    dir_path = sys.argv[1]

    if not os.path.isdir(dir_path):                   # valida che l'argomento sia una directory esistente
        print(f"Errore: '{dir_path}' non è una directory valida.", file=sys.stderr)
        sys.exit(1)

    totals = []                                        # totals[i] = somma caratteri della riga (i+1) su tutti i file

    for entry in os.listdir(dir_path):
        full_path = os.path.join(dir_path, entry)
        if not os.path.isfile(full_path):              # considera solo i file regolari della directory
            continue

        with open(full_path, "r") as f:
            for i, line in enumerate(f):
                line = line.rstrip("\n")                # il newline non è un carattere del testo della riga
                if i >= len(totals):
                    totals.append(0)
                totals[i] += len(line)

    for i, total in enumerate(totals):
        print(i + 1, total)


if __name__ == "__main__":
    main()
