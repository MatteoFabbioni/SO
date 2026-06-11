#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: (10 punti)
Scrivere uno script o un programma python che corregga l'indentazione di tutti i file .c e .h presenti
nel sottoalbero della directory passata come parametro (la working directory se non vi sono
parametri).
Hint: il comando:
ex -n '+norm!gg=G' +wq prog.c
corrregge l'indentazione del programma sorgente C prog.c.
"""

import os
import sys
import subprocess

EX_COMMAND = "+norm!gg=G"


def fix_indentation(path):
    result = subprocess.run(["ex", "-n", EX_COMMAND, "+wq", path])
    if result.returncode != 0:
        print(f"{path}: ex ha restituito codice {result.returncode}", file=sys.stderr)


def main():
    if len(sys.argv) > 2:
        print(f"Uso: {sys.argv[0]} [directory]", file=sys.stderr)
        sys.exit(1)

    root = sys.argv[1] if len(sys.argv) == 2 else os.getcwd()

    if not os.path.isdir(root):
        print(f"{sys.argv[0]}: '{root}' non e' una directory valida", file=sys.stderr)
        sys.exit(1)

    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            if name.endswith(".c") or name.endswith(".h"):
                fix_indentation(os.path.join(dirpath, name))


if __name__ == "__main__":
    main()
