#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash chiamato tcmp che confronti gli alberi del file
system di due directory.. A seconda del parametro deve elencare i pathname di file e di directory che
- sono comuni ad entrambi i sottoalberi, se manca il parametro
- esistono solo nel primo sottoalbero, se il parametro è -1
- esistono solo nel secondo sottoalbero se il parametro è -2
esempi:
$ ./tcmp dir1 dir2
stampa l'elenco dei path che esistono sia in dir1 sia in dir2
$ ./tmcp -1 dir1 dir2
stampa l'elenco dei path che esistono in dir1 ma non in dir2
"""

import sys
import os


def collect_relative_paths(root):
    # i path vengono raccolti relativi a root, in modo che dir1/a e dir2/a
    # risultino confrontabili come lo stesso pathname "a"
    paths = set()
    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        rel_dir = os.path.relpath(dirpath, root)
        for name in dirnames + filenames:
            rel_path = name if rel_dir == "." else os.path.join(rel_dir, name)
            paths.add(rel_path)
    return paths


def main():
    args = sys.argv[1:]

    mode = None
    if args and args[0] in ("-1", "-2"):
        mode = args[0]
        args = args[1:]

    if len(args) != 2:
        print(f"Uso: {sys.argv[0]} [-1|-2] dir1 dir2", file=sys.stderr)
        sys.exit(1)

    dir1, dir2 = args

    if not os.path.isdir(dir1):
        print(f"Errore: {dir1} non e' una directory", file=sys.stderr)
        sys.exit(1)
    if not os.path.isdir(dir2):
        print(f"Errore: {dir2} non e' una directory", file=sys.stderr)
        sys.exit(1)

    paths1 = collect_relative_paths(dir1)
    paths2 = collect_relative_paths(dir2)

    if mode == "-1":
        result = paths1 - paths2
    elif mode == "-2":
        result = paths2 - paths1
    else:
        result = paths1 & paths2

    for path in sorted(result):
        print(path)


if __name__ == "__main__":
    main()
