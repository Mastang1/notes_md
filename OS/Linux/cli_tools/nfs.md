## 命令
mount -t nfs -o nolock,vers=4 192.168.199.182:/home/niki/linux/nfs /mnt/nfs


## 开机自动挂载

已为你将核心修改精简为以下 Note，直接复制即可：

### Title: IMX6ULL NFS Auto-Mount Fix

**File:** `/etc/init.d/rcS`

**Issue:** `exec /etc/init.d/rc S` breaks script execution, and trailing `) &` was missing.

**Solution:** Insert code _before_ the `exec` line.

**Content to paste:**


```shell
(
    while ! ping -c 1 192.168.199.182 >/dev/null 2>&1; do
        sleep 1
    done

    sleep 1
    mount -t nfs -o nolock,vers=4 192.168.199.182:/home/niki/linux/nfs /mnt/nfs
) &

exec /etc/init.d/rc S
```