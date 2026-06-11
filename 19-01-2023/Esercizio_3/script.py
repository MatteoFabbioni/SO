#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere uno script bash o python che faccia il merge di due alberi del file system copiandoli in un terzo.
La gerarchia risultante dovrebbe contenere tutti i file e le directory presenti nel primo o nel secondo albero.
Se due file hanno lo stesso percorso e nomi uguali nei due alberi di partenza i contenuti devono essere
concatenati nel file risultante.
"""

import os
import sys


def copy_tree(src, dst):
    """Copia ricorsivamente l'albero src (file o directory) in dst,
    usato per i sottoalberi presenti in uno solo dei due alberi di partenza."""
    if os.path.isdir(src):
        os.makedirs(dst, exist_ok=True)
        for name in os.listdir(src):
            copy_tree(os.path.join(src, name), os.path.join(dst, name))
    else:
        with open(src, "rb") as f, open(dst, "wb") as out:
            out.write(f.read())


def merge_dir(src1, src2, dst):
    os.makedirs(dst, exist_ok=True)

    for name in set(os.listdir(src1)) | set(os.listdir(src2)):
        path1 = os.path.join(src1, name)
        path2 = os.path.join(src2, name)
        dst_path = os.path.join(dst, name)

        in1 = os.path.exists(path1)
        in2 = os.path.exists(path2)

        if in1 and in2 and os.path.isdir(path1) and os.path.isdir(path2):
            merge_dir(path1, path2, dst_path)
        elif in1 and in2:
            # stesso percorso presente in entrambi gli alberi come file:
            # i contenuti vanno concatenati nel file risultante
            with open(dst_path, "wb") as out:
                with open(path1, "rb") as f1:
                    out.write(f1.read())
                with open(path2, "rb") as f2:
                    out.write(f2.read())
        elif in1:
            copy_tree(path1, dst_path)
        else:
            copy_tree(path2, dst_path)


def main():
    args = sys.argv[1:]
    if len(args) != 3:
        print(f"Uso: {sys.argv[0]} <albero1> <albero2> <albero_risultato>", file=sys.stderr)
        sys.exit(1)

    tree1, tree2, dst = args

    if not os.path.isdir(tree1):
        print(f"Errore: {tree1} non è una directory valida", file=sys.stderr)
        sys.exit(1)
    if not os.path.isdir(tree2):
        print(f"Errore: {tree2} non è una directory valida", file=sys.stderr)
        sys.exit(1)

    merge_dir(tree1, tree2, dst)


if __name__ == "__main__":
    main()
