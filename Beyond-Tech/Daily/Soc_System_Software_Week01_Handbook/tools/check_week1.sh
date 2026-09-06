#!/usr/bin/env bash
set -u

pass=0
fail=0

check_cmd() {
    local cmd="$1"
    if command -v "$cmd" >/dev/null 2>&1; then
        printf '[PASS] command: %s\n' "$cmd"
        pass=$((pass+1))
    else
        printf '[FAIL] command: %s\n' "$cmd"
        fail=$((fail+1))
    fi
}

for c in git gcc arm-linux-gnueabihf-gcc readelf objdump gdb-multiarch \
         ssh tmux picocom tcpdump cmake ninja python3 dtc; do
    check_cmd "$c"
done

if systemctl is-active --quiet ssh; then
    echo "[PASS] ssh service"
    pass=$((pass+1))
else
    echo "[FAIL] ssh service"
    fail=$((fail+1))
fi

if systemctl is-active --quiet tftpd-hpa; then
    echo "[PASS] tftpd-hpa service"
    pass=$((pass+1))
else
    echo "[FAIL] tftpd-hpa service"
    fail=$((fail+1))
fi

if systemctl is-active --quiet nfs-kernel-server; then
    echo "[PASS] nfs-kernel-server service"
    pass=$((pass+1))
else
    echo "[FAIL] nfs-kernel-server service"
    fail=$((fail+1))
fi

echo
echo "PASS=$pass FAIL=$fail"
test "$fail" -eq 0
