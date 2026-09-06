#!/usr/bin/env bash
set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <zImage> <dtb>" >&2
    exit 2
fi

KERNEL="$1"
DTB="$2"

test -f "$KERNEL"
test -f "$DTB"

sudo install -m 0644 "$KERNEL" /srv/tftp/zImage
sudo install -m 0644 "$DTB" /srv/tftp/imx6ull-alientek-emmc.dtb

echo "=== staged ==="
ls -lh /srv/tftp/zImage /srv/tftp/imx6ull-alientek-emmc.dtb

echo
echo "=== sha256 ==="
sha256sum /srv/tftp/zImage /srv/tftp/imx6ull-alientek-emmc.dtb
