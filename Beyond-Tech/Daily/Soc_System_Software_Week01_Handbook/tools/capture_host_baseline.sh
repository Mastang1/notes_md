#!/usr/bin/env bash
set -eu

OUT="${1:-$HOME/work/logs/host_baseline_$(date +%Y%m%d_%H%M%S).txt}"
mkdir -p "$(dirname "$OUT")"

{
    echo "===== timestamp ====="
    date -Is
    echo
    echo "===== hostnamectl ====="
    hostnamectl
    echo
    echo "===== uname ====="
    uname -a
    echo
    echo "===== os-release ====="
    cat /etc/os-release
    echo
    echo "===== cpu/memory ====="
    echo "nproc=$(nproc)"
    free -h
    echo
    echo "===== storage ====="
    lsblk
    df -h /
    echo
    echo "===== network ====="
    ip -br addr
    ip route
    echo
    echo "===== tools ====="
    git --version || true
    gcc --version | head -1 || true
    arm-linux-gnueabihf-gcc --version | head -1 || true
    cmake --version | head -1 || true
    python3 --version || true
    gdb-multiarch --version | head -1 || true
} | tee "$OUT"

echo "Saved: $OUT"
