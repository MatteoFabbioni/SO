#!/usr/bin/env python3
import os
import sys

def main():
    # 1. Parsing dell'argomento
    max_depth = -1 # -1 indica che non c'è limite di profondità
    
    if len(sys.argv) == 2:
        try:
            max_depth = int(sys.argv[1])
            if max_depth < 0:
                raise ValueError
        except ValueError:
            print("Errore: la profondità deve essere un numero intero >= 0.")
            sys.exit(1)
    elif len(sys.argv) > 2:
        print("Uso: python inotab.py [profondita]")
        sys.exit(1)

    result = []
    
    # 2. Aggiungiamo manualmente la directory corrente (".") per evitare duplicati dopo
    root_dir = "."
    result.append((os.stat(root_dir).st_ino, root_dir))

    # 3. Scansione del file system
    for root, dirs, files in os.walk(root_dir):
        
        # Calcolo della profondità attuale basandosi sul numero di separatori (es. '/' o '\')
        # "." -> 0
        # "./dir1" -> 1
        # "./dir1/dir2" -> 2
        curr_depth = root.count(os.sep)

        # Aggiungiamo i file della directory corrente
        for f in files:
            full_path = os.path.join(root, f)
            result.append((os.stat(full_path).st_ino, full_path))
            
        # Aggiungiamo le sottodirectory della directory corrente
        for d in dirs:
            full_path = os.path.join(root, d)
            result.append((os.stat(full_path).st_ino, full_path))

        # Se abbiamo raggiunto la profondità massima, svuotiamo 'dirs'
        # Questo impedisce a os.walk di scendere ulteriormente
        if max_depth != -1 and curr_depth >= max_depth:
            dirs.clear()

    # 4. Ordinamento e Stampa
    result.sort(key=lambda x: x[0]) # Ordina in base all'i-node (primo elemento della tupla)

    for inode, path in result:
        print(f"{inode} {path}")

if __name__ == "__main__":
    main()