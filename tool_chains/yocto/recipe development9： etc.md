## 1. external build后clean local objects 测试这个命令有效
```shell
do_install_append(){
bbnote "hello"
cd ${B}
oe_runmake clean
}
```

## 2. recipe(本质py脚本) 和 make的shell指令参数  实现kel_ser获取
_**说明如下两行的执行顺序，并说明原因：
	KNE_VER = "$(cat ${STAGING_KERNEL_BUILDDIR}/include/config/kernel.release)"
	DRIVER_DIR = "${STAGING_INCDIR}/modules/${KNE_VER}/extra"
**_

这是一个非常精妙的问题！要彻底搞懂这两行代码的“执行顺序”，我们必须跳出普通编程语言（如 C 或 Python）中“从上到下逐行执行”的线性思维，切换到 **Yocto 的双层解析模型**（BitBake 解析期 vs Bash 执行期）。

**核心结论：**

这两行代码在 BitBake 读取它们时，**根本没有发生任何实际的 `cat` 文件读取操作**。真正的执行顺序是：**BitBake 先完成纯文本的“字符串拼接”，最后交由底层 Bash Shell 在任务运行时进行“命令替换”并得出最终路径。**

下面为你深度拆解整个发生过程以及背后的根本原因。

### 一、 真实执行顺序拆解

我们可以把这段代码的生命周期分为两个阶段：

#### 阶段 1：BitBake 解析期（文本拼接）

当你在终端敲下 `bitbake app-module` 时，BitBake（基于 Python 开发）开始读取你的 Recipe 文件。

1. **处理第一行：**
    
    `KNE_VER = "$(cat ${STAGING_KERNEL_BUILDDIR}/include/config/kernel.release)"`
    
    - **动作：** BitBake 看到 `${STAGING_KERNEL_BUILDDIR}`，这是它认识的内部变量，于是把它展开为一个真实路径（比如 `/path/to/kernel-build`）。
        
    - **结果：** `KNE_VER` 在 BitBake 内存中被保存为一段纯文本字符串：`"$(cat /path/to/kernel-build/include/config/kernel.release)"`。
        
    - **关键：** BitBake 绝对不会去执行 `$(cat ...)`，因为它不认识 Bash 语法，它只把这当成一串长得很奇怪的字符。
        
2. **处理第二行：**
    
    `DRIVER_DIR = "${STAGING_INCDIR}/modules/${KNE_VER}/extra"`
    
    - **动作：** BitBake 看到 `=` 赋值符（这意味着延迟求值，用到时才完全展开）。当后续任务需要使用 `DRIVER_DIR` 时，BitBake 会把刚才存起来的 `KNE_VER` 字符串“塞”进去。
        
    - **结果：** `DRIVER_DIR` 最终在 BitBake 层面变成了一个更长的嵌套字符串：
        
        `"/path/to/sysroot/usr/include/modules/$(cat /path/to/kernel-build/include/config/kernel.release)/extra"`
        

#### 阶段 2：Bash 执行期（命令执行）

假设你在 `EXTRA_OEMAKE` 中使用了 `${DRIVER_DIR}`，并且执行到了 `do_compile` 任务。此时，BitBake 会生成一个 Bash 脚本并交给 Linux 系统运行。

在 Bash 的视角里，它看到的是这样一行命令：

`make IPCS_DRIVER_DIR=/path/to/sysroot/usr/include/modules/$(cat /path/to/kernel-build/include/config/kernel.release)/extra`

此时，**真正的执行顺序（由 Bash 接管）才正式开始：**

1. **命令替换 (Command Substitution)：** Bash 在执行整个 `make` 命令之前，扫描到了 `$()` 语法结构。它会**优先**去系统文件系统里执行这个子命令。
    
2. **执行 cat：** Bash 执行 `cat /path/to/kernel-build/include/config/kernel.release`，读取文件内容，得到字符串（比如 `5.15.0-xxx`）。
    
3. **结果替换：** Bash 把读取到的版本号拼接到路径中，形成最终参数：
    
    `IPCS_DRIVER_DIR=/path/to/sysroot/usr/include/modules/5.15.0-xxx/extra`
    
4. **触发 Make：** Bash 将这个完全解析好的路径参数，传递给底层 `make` 程序，完成最终编译。
    

### 二、 为什么是这样设计的？（原理说明）

这种神奇的设计是由 BitBake 变量操作符和 Shell 语法的天然差异决定的：

#### 1. 语法维度的降维打击：`${}` vs `$()`

- **`${...}` 是 BitBake 的地盘**：在解析期，只要带着花括号，BitBake 就会强行展开它（比如把 `${STAGING_KERNEL_BUILDDIR}` 变成绝对路径）。
    
- **`$(...)` 是 Bash 的地盘**：BitBake 对它视而不见，原样保留。只有当这段字符串被丢给底层的 Linux Bash 执行时，它才会被识别为“命令替换”并执行。
    

#### 2. 时序保护机制

如果在解析期 BitBake 就尝试去执行这段代码（比如你错误地使用了 Python 内联语法 `${@oe.utils.read_file(...)}`），它必定会崩溃报错。

因为在解析期，内核根本还没开始编译，`kernel.release` 这个文件在硬盘上是不存在的。

通过把获取版本号的任务伪装成 `$()`，我们成功地**骗过了 BitBake 的解析期检查**，把它像一个“定时炸弹”一样，安全地运送到了 `do_compile` 的 Bash 执行期。等到了执行期，由于 `DEPENDS = "virtual/kernel"` 的保护，内核已经编译完，文件已经存在，Bash 此时引爆（执行 `cat`），完美拿到想要的数据。

### 总结

大神的这种写法，本质上是一门“甩锅的艺术”**： 第一行和第二行并不是谁先计算谁后计算的关系，而是 BitBake 把**文本模板（KNE_VER）**像拼积木一样塞进了另一个**文本模板（DRIVER_DIR）里，最后整体打包，把真正的计算工作全部“甩锅”给了底层的 Bash 解析器。
