#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: (10 punti)
Scrivere un programma python o uno script bash che consenta di lanciare parallelamente comandi

para.py sleep 20 // ls -l // ps // sleep 3
para.sh sleep 20 // ls -l // ps // sleep 3

deve eseguire concorrentemente i vari comandi separati da // e aspettare la terminazione di tutti
"""

import sys
import subprocess

def main():
    args = sys.argv[1:]
    if not args:
        print(f"Uso: {sys.argv[0]} cmd1 [arg...] // cmd2 [arg...] // ...", file=sys.stderr)
        sys.exit(1)

    # spezza la lista di argomenti in piu' comandi usando "//" come separatore,
    # cosi' come avviene per i token sulla riga di comando di para.sh
    commands = []
    current = []
    for arg in args:
        if arg == "//":
            commands.append(current)
            current = []
        else:
            current.append(arg)
    commands.append(current)

    commands = [c for c in commands if c]
    if not commands:
        print("Errore: nessun comando specificato", file=sys.stderr)
        sys.exit(1)

    # subprocess.Popen (non os.system/subprocess.run) avvia ogni comando senza
    # attendere il precedente: serve per ottenere l'esecuzione concorrente
    processes = []
    for cmd in commands:
        try:
            proc = subprocess.Popen(cmd)
            processes.append(proc)
        except OSError as e:
            print(f"Errore nell'avvio di {cmd}: {e}", file=sys.stderr)

    for proc in processes:
        proc.wait()

if __name__ == "__main__":
    main()
