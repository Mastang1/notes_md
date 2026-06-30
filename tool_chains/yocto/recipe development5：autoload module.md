
## 1. How to DO
```shell
# 告诉 Yocto，把这两个模块加入开机自启列表
# 注意这里写的是模块名（不要加 .ko 后缀）
KERNEL_MODULE_AUTOLOAD += "lib app"
```

## 2.  analysis

### 1. 编译期的魔法：`KERNEL_MODULE_AUTOLOAD`

`KERNEL_MODULE_AUTOLOAD` 是 Yocto 特有的一个变量。严格来说，Linux 内核和 Systemd 根本不认识这个东西，它是 Yocto 为了方便开发者而发明的“语法糖”。

当你在一份 Recipe 中写下 `KERNEL_MODULE_AUTOLOAD += "lib app"` 时，Yocto 在后台会执行以下逻辑：

- **拦截：** Yocto 的打包脚本（`rootfs` 生成阶段）看到了这个变量。
    
- **翻译与生成：** 它会在最终的系统镜像（也就是 `${D}` 目录下的对应位置）中，**自动帮你新建一个文本文件**，通常命名为 `/etc/modules-load.d/<配方名>.conf`。
    
- **写入内容：** 它会把你写的 "lib" 和 "app" 一行一个地写进这个 `.conf` 文件里。
    

**总结：** 这个变量的本质，就是 Yocto 替你自动完成了“创建配置文件”的体力活。

### 2. 运行期的执行者：`/etc/modules-load.d/`

这个目录是现代 Linux（依赖 **Systemd** 的系统）管理内核模块开机自启的标准大本营。

- **它是干什么的？** 在这个目录下，你可以存放任意数量的、以 `.conf` 结尾的文本文件。每个文件里面只写内核模块的名字（不需要带 `.ko` 后缀，也不需要写绝对路径）。
    
- **它是怎么工作的？** 当 Linux 系统开机，Systemd 接管系统后，会启动一个专门的系统服务：`systemd-modules-load.service`。 这个服务唯一的任务，就是去遍历 `/etc/modules-load.d/` 目录下的所有 `.conf` 文件，读取里面的模块名字，然后依次对它们执行后台加载动作（相当于在底层悄悄对每个名字执行了一次 `modprobe`）。
    

因为底层的 `modprobe` 能够识别我们在前面提到的 `modules.dep` 依赖树，所以只要名字写进去了，哪怕顺序写反了（比如先写了 app，后写了 lib），系统也能聪明地先把 lib 加载起来，再加载 app。