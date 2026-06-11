#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere uno script bash o python difidr che date due directory ne crei una terza e una quarta
Le nuove directory devono contenere solamente i file aventi lo stesso nome presenti nella prima e nella seconda
albero. Ogni file della terza directory deve contenere una copia del file nella versione della prima directory,
mentre nella quarta directory la versione della seconda.
es.
* se la direcotry a contiene i file alpha, beta, gamma, delta e la directory b i file beta, delta, epsilon, zeta
il comando "difdir a b newa newb" crea le directory newa e newb ed entrambe le directory devono
contenere solo beta e delta (i nomi in comune). newa/beta deve essere una copia di a/beta mentre
newb/beta una copia di b/beta. In modo simile per a/delta b/delta newa/delta e newb/delta.
"""

import os
import sys
import shutil


def collect_files(root_dir):
    # path relativo -> path assoluto, cosi' il confronto tra i due alberi
    # si basa sul nome/posizione del file e non sul path assoluto
    files = {}
    for current_root, _dirs, filenames in os.walk(root_dir):
        for name in filenames:
            full_path = os.path.join(current_root, name)
            rel_path = os.path.relpath(full_path, root_dir)
            files[rel_path] = full_path
    return files


def main():
    args = sys.argv[1:]
    if len(args) != 4:
        print("Usage: script.py <dirA> <dirB> <newA> <newB>")
        sys.exit(1)

    dir_a, dir_b, new_a, new_b = args

    if not os.path.isdir(dir_a):
        print(f"Error: {dir_a} is not a valid directory")
        sys.exit(1)
    if not os.path.isdir(dir_b):
        print(f"Error: {dir_b} is not a valid directory")
        sys.exit(1)

    files_a = collect_files(dir_a)
    files_b = collect_files(dir_b)

    common = sorted(set(files_a) & set(files_b))

    os.makedirs(new_a, exist_ok=True)
    os.makedirs(new_b, exist_ok=True)

    for rel_path in common:
        dest_a = os.path.join(new_a, rel_path)
        dest_b = os.path.join(new_b, rel_path)
        os.makedirs(os.path.dirname(dest_a) or ".", exist_ok=True)
        os.makedirs(os.path.dirname(dest_b) or ".", exist_ok=True)

        try:
            shutil.copy2(files_a[rel_path], dest_a)
            shutil.copy2(files_b[rel_path], dest_b)
        except OSError as e:
            print(f"Error copying {rel_path}: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()
