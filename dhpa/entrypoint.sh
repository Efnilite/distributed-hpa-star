#!/bin/bash
# dhpa/entrypoint.sh

# Read worker count (default to 1 if not provided)
N=${WORKER_COUNT:-1}

# k factor: how much channel noise/traffic each worker contributes
K=0.2

# Calculate loss percentage: (1 - e^(-K * N)) * 100
# 'e()' is the built-in exponential function in 'bc -l'
PERCENT=$(echo "scale=4; (1 - e(-$K * $N)) * 100" | bc -l)

echo "=================================================="
echo " Wireless Network Emulation (Collision Model)"
echo " Active Worker Count (N) = $N"
echo " Calculated Packet Loss  = ${PERCENT}%"
echo "=================================================="

# Apply packet loss and packet corruption to simulate wireless fading/collisions
tc qdisc add dev eth0 root netem loss ${PERCENT}% corrupt ${PERCENT}%

# Execute the main application command
exec "$@"