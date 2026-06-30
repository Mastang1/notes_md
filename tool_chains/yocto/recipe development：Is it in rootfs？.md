## 1. search in path
路径规则：`build/tmp/work/<机器名>/<镜像配方名>/<版本号>/rootfs/`

## 2. 进阶技巧：查看 Image Manifest (清单文件)

cd build/tmp/deploy/images/<机器名>/
找到以 `.manifest` 结尾的文件（例如 `core-image-minimal-imx8.manifest`）。

search the module name

## 3.  add to Image method
如果在builld path 校验找到了，但在这里**找不到**，这是最经典的错误。说明你忘了在你的 `local.conf` 或 `core-image-minimal.bbappend` 中添加：

代码段

```
IMAGE_INSTALL_append = " my-daemon"
```

