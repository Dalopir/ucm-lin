#!/bin/bash
PROC=/proc/modlist

echo "cleanup" > "$PROC"
pids=()
for p in 1 2 3 4; do
    (
        for i in $(seq 1 7); do
            echo "add $((p * 100 + i))" > "$PROC"
        done
    ) &
    pids+=($!)
done
for pid in "${pids[@]}"; do wait "$pid"; done
echo $(cat "$PROC")

