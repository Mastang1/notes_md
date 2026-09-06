#!/usr/bin/env bash
set -eu

ROOT="${1:-$HOME/work/linux/bsp/imx6ull}"
OUT="$ROOT/notes/bsp_host_baseline_$(date +%Y%m%d_%H%M%S).txt"

mkdir -p "$(dirname "$OUT")"

{
    echo "===== TIME ====="
    date -Is

    echo
    echo "===== HOST ====="
    hostnamectl
    uname -a

    echo
    echo "===== TOOLCHAINS ====="
    command -v arm-linux-gnueabihf-gcc || true
    arm-linux-gnueabihf-gcc --version 2>/dev/null | head -1 || true
    command -v arm-poky-linux-gnueabi-gcc || true
    arm-poky-linux-gnueabi-gcc --version 2>/dev/null | head -1 || true

    echo
    echo "===== SERVICES ====="
    systemctl is-active tftpd-hpa || true
    systemctl is-active nfs-kernel-server || true

    echo
    echo "===== NETWORK ====="
    ip -br addr
    ip route

    echo
    echo "===== TFTP ====="
    ls -lah /srv/tftp 2>/dev/null || true

    echo
    echo "===== NFS ====="
    showmount -e localhost 2>/dev/null || true
} | tee "$OUT"

echo "Saved: $OUT"
