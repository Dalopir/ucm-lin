#!/bin/bash

DEV=/dev/prodcons
TOTAL=12
WAIT_FILL=5

cap=$1

echo "=== Buffer de $cap enteros ==="

# Productor en segundo plano: escribe un numero cada 0.5 s
(
    for i in $(seq 1 $TOTAL); do
        echo "$i" > "$DEV"
        echo "  productor -> $i"
        sleep 0.5
    done
) &
prod=$!

# Dejamos tiempo para que el buffer se llene y el productor se bloquee
sleep $WAIT_FILL
echo "  productor bloqueado tras $stored escrituras (capacidad $cap)"

# Consumidor en segundo plano: lee un numero cada 0.5 s
(
    for i in $(seq 1 $TOTAL); do
        v=$(cat "$DEV")
        echo "  consumidor <- $v"
        sleep 0.5
    done
) &
cons=$!

wait "$prod"
wait "$cons"
