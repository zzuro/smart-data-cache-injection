#!/bin/bash
for cpu in $(seq 0 31); do
    echo "Running ./um on CPU $cpu"
    sudo taskset -c $cpu ./um &
done
wait
echo "All tasks finished."
