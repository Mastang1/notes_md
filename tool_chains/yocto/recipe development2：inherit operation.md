欢迎回来。现在我们正式进入 **Topic 3：菜谱的三六九等（Recipe 类型）——揭秘 `inherit` 继承魔法**。

在上一课中，我们知道了怎么通过显性重写 `do_compile` 和 `do_install` 来“手把手”教厨房炒菜。但如果你去翻看 Yocto 官方或者 NXP/TI 提供的 Recipe，你会发现很多上百兆的复杂开源库，它的 `.bb` 文件居然只有寥寥 5 行！连一句 `make` 都没写。

这就引出了 Yocto 中最强大的代码复用机制：**类（Class）继承**。

### 📖 概念重塑：什么是 `inherit`？

**费曼类比：**

假设你的中央厨房要接一单“满汉全席”，里面有 50 道炒菜、30 道炖菜、20 道凉菜。

如果每写一张菜谱，都要重新把“怎么切葱姜蒜”、“怎么点火”、“怎么装盘”写一遍，写菜谱的人会疯掉，而且极容易出错。

Yocto 的做法是：把通用的烹饪套路，提取成一个个“标准工序模板”（`.bbclass` 文件）。

当你在菜谱里写下一句 `inherit cmake`（继承 CMake 模板）时，相当于告诉厨房：“这道菜是一道标准的‘爆炒类’菜品，具体怎么起锅烧油，你直接去参考《爆炒标准规范》，我只给你提供主料。”

在 Yocto 的底层，`inherit xxx` 实际上就是**自动帮你把 `do_configure`、`do_compile` 和 `do_install` 等函数全部重写成了该类型的标准行为**。你什么都不用操心了。

### 🛠️ 核心实战：最常用的 4 大魔法模板

作为底层/中间件工程师，你日常打交道的 Recipe，90% 都逃不出以下 4 种模板。

#### 1. 传统 C/C++ 巨无霸模板：`inherit autotools`

- **适用场景：** 你下载了一个开源软件，它的标准编译动作是 `./configure && make && make install`。
    
- **底层机制：** 只要你写了这句，Yocto 就会自动帮你执行这一长串动作，并且会自动把交叉编译器（`--host=aarch64-linux` 等参数）注入进去。
    
- **如何微调（调参）：** 如果你想在 `./configure` 时加个参数（比如关闭某个功能），只需在 `.bb` 里定义：
    
    代码段
    
    ```
    EXTRA_OECONF = "--disable-ipv6"
    ```
    

#### 2. 现代 C/C++ 模板：`inherit cmake`

- **适用场景：** 源码目录下有一个 `CMakeLists.txt`。
    
- **底层机制：** Yocto 会自动生成正确的 CMake 工具链文件（Toolchain file），确保 CMake 不会调成你 Ubuntu 宿主机的 `gcc`。它会执行 `cmake ..`，然后调用 `make`。
    
- **如何微调（调参）：**
    
    代码段
    
    ```
    EXTRA_OECMAKE = "-DBUILD_SHARED_LIBS=ON"
    ```
    

#### 3. 你的老朋友（内核驱动模板）：`inherit module`

回忆一下咱们之前用 `devtool add` 生成的那个 `hello_drv.bb`，里面必定有这一句。

- **适用场景：** 编译独立于内核源码树之外的驱动代码（Out-of-tree Kernel Module）。
    
- **底层机制（极其硬核）：**
    
    这是 Yocto 最贴心的模板之一。写了这句，Yocto 会：
    
    1. 自动强制依赖 `virtual/kernel`，确保内核先编译完。
        
    2. 自动把内核的源码路径（`KERNEL_SRC`）、交叉编译架构（`ARCH`）、符号表注入给你的 Makefile。
        
    3. 最神奇的是：它重写了 `do_install`，**自动把编译出来的 `.ko` 文件安装到目标系统的 `/lib/modules/<内核版本>/extra/` 目录下！** 你根本不需要手动写 `install` 语句。
        

#### 4. 开机自启守护神：`inherit systemd`

- **适用场景：** 你写了一个后台监控程序，希望 Linux 开机就自动运行。
    
- **底层机制：** 只要你继承了它，Yocto 就会在打包镜像时，自动调用 `systemctl enable` 将你的服务注册进去。
    
- **必备配合变量：**
    
    代码段
    
    ```
    inherit systemd
    # 告诉 Yocto 你的 service 文件叫什么名字
    SYSTEMD_SERVICE_${PN} = "my_app.service"
    # 是否开机自启（默认就是 enable）
    SYSTEMD_AUTO_ENABLE = "enable"
    ```
    
    _(注：你仍然需要在 `do_install` 中把 `my_app.service` 拷贝到 `${D}${systemd_system_unitdir}` 下，这属于物理文件的移动，`inherit` 只负责注册逻辑。)_
    

### 💡 避坑指南：继承的覆盖原则

新手经常犯的错误是：**既继承了模板，又手贱重写了函数，导致模板失效。**

**错误示范：**

代码段

```
inherit autotools

# 你自作聪明写了这句，覆盖了 autotools 自动生成的复杂编译逻辑！
do_compile() {
    make -j4
}
```

**正确做法（追加机制）：**

如果你觉得模板做得不够，想在它编译**之后**，再额外做点事情（比如拷贝个配置文件），你应该使用 **`_append`** 追加机制：

代码段

```
inherit cmake

# 告诉 Yocto：在执行完标准的 cmake 安装步骤后，再执行我追加的这几行
do_install_append() {
    install -d ${D}${sysconfdir}
    install -m 0644 ${WORKDIR}/my.conf ${D}${sysconfdir}/
}
```

现在，你已经掌握了如何用 `inherit` 召唤强大的预设模板。但你可能会发现，在写 `do_install`（特别是追加安装）时，变量里总是充斥着 `${S}`、`${B}`、`${D}` 这些天书一样的缩写字母，一旦写错，文件就不知道飞到哪里去了。

准备好进入 **Topic 4：最折磨人的“空间魔法”（彻底搞懂构建路径）** 了吗？