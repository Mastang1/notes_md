# Week 2 Remote GDB Demo

Host:

```bash
make
file hello_dbg
cp hello_dbg /srv/nfs/imx6ull/
```

Target:

```bash
cd /mnt/nfs
gdbserver :2345 ./hello_dbg
```

Host:

```bash
gdb-multiarch ./hello_dbg
```

GDB:

```gdb
set architecture arm
target remote <target-ip>:2345
break main
continue
next
print counter
step
backtrace
```
