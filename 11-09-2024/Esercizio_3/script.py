#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma Python o uno script bash slinout che elenchi tutti i link simbolici presenti nel
sottoalbero del file system che ha come radice la directory passata come parametro (o la current
working directory se slinout viene chiamato senza parametri).
I link simbolici devono essere suddivisi in interni, che cioè puntano ad altro file o directory nel
sottoalbero considerato, o esterni, che cioè indicano un file o directory al di fuori del sotoalbero.
(attenzione: il target dei link simbolici può essere assoluto o relativo)
"""

import os
import sys


def is_internal(link_path, root_real):
    """Un link e' interno se il target, risolto rispetto alla directory che lo
    contiene, cade dentro l'albero radicato in root_real (link a target inesistenti
    sono considerati esterni: os.path.realpath non segnala l'errore ma il risultato
    non puo' comunque trovarsi sotto root_real in modo significativo)."""
    target_real = os.path.realpath(link_path)
    return target_real == root_real or target_real.startswith(root_real + os.sep)


def main():
    if len(sys.argv) > 2:
        print(f"Uso: {sys.argv[0]} [directory]", file=sys.stderr)
        sys.exit(1)

    root = sys.argv[1] if len(sys.argv) == 2 else os.getcwd()

    if not os.path.isdir(root):
        print(f"{sys.argv[0]}: '{root}' non e' una directory valida", file=sys.stderr)
        sys.exit(1)

    root_real = os.path.realpath(root)

    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        for name in dirnames + filenames:
            full_path = os.path.join(dirpath, name)
            if os.path.islink(full_path):
                categoria = "interno" if is_internal(full_path, root_real) else "esterno"
                print(f"{full_path} [{categoria}]")


if __name__ == "__main__":
    main()
