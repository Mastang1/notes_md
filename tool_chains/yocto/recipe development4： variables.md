在进入最终的“徒手搓轮子”实战之前，我们必须先跨越 Yocto 新手最容易摔断腿的“空间迷宫”。如果不搞懂这些路径变量，你的代码编译出来也会像幽灵一样在系统里找不到。

### 📖 终极避坑指南：Yocto 核心路径变量详解 (Topic 4 融合)

在 Yocto 的世界里，绝对禁止使用宿主机的绝对路径（如 `/usr/bin` 或 `/etc`）。一切操作都在隔离的“沙盒”中进行。以下是你写 Recipe 时每天都要打交道的空间坐标：

#### 1. 四大核心沙盒目录 (编译生命周期)

|**变量名**|**全称释义**|**核心作用与场景**|**你的视角 (类比)**|
|---|---|---|---|
|**`${WORKDIR}`**|Work Directory|当前配方的**绝对宇宙中心**。Yocto 为这个配方创建的专属临时根目录。|厨房给你分配的专属操作台。|
|**`${S}`**|Source Directory|**源码解压/存放地**。`do_fetch` 和 `do_unpack` 后的代码都在这。|案板（放切好的菜）。|
|**`${B}`**|Build Directory|**编译输出地**。执行 `make` 时所在的目录，生成的 `.o` 和二进制都在这。|炒锅（加热食材的地方）。<br><br>  <br><br>_(注：默认情况下 `${S}` 和 `${B}` 是同一个目录。但如果继承了 `cmake`，Yocto 会强制分离，`${B}` 会在 `${S}` 之外单独建立。)_|
|**`${D}`**|Destination Directory|**虚拟根文件系统 (装盘区)**。`do_install` **必须且只能**把文件放进这里，它模拟了最终开发板上的根目录 `/`。|盘子。如果不放进 `${D}`，最终打包镜像时这道菜就被扔进垃圾桶了。|

#### 2. 系统标准路径变量 (用于装盘定位)

当你往 `${D}` 里拷贝文件时，Yocto 强烈要求你使用内置变量，而不是硬编码（如 `${D}/etc/`），因为不同架构的系统路径可能不同。

|**Yocto 变量**|**对应的最终系统绝对路径**|**通常用来放什么**|
|---|---|---|
|**`${bindir}`**|`/usr/bin`|用户可执行程序（App）。|
|**`${sbindir}`**|`/usr/sbin`|系统级管理工具、守护进程。|
|**`${libdir}`**|`/usr/lib`|动态库（`.so` 文件）。|
|**`${sysconfdir}`**|`/etc`|配置文件（`.conf`、`.json`）。|
|**`${includedir}`**|`/usr/include`|C/C++ 头文件（`.h`）。|
|**`${systemd_system_unitdir}`**|`/lib/systemd/system`|Systemd 开机自启服务文件（`.service`）。|
|**`${base_libdir}`**|`/lib`|基础内核模块目录（如驱动放在 `/lib/modules/`）。|

**实战公式：** **`手写源文件 (来自 ${S} 或 ${WORKDIR})` ➔ `install` ➔ `${D} + 系统标准路径变量`**

### 📖 Topic 5 终极实战：从零徒手搓一个完整的 Recipe

现在，忘掉所有的高级模板。假设你接到了一个全新的开发需求：

**“写一个 C 语言的守护进程 `my-daemon`，它需要读取 `/etc/my-daemon.conf` 的配置，并且要求 Linux 开机自动在后台运行。”**

你手里只有三个自己写的文件：

1. `main.c` (源码)
    
2. `my-daemon.conf` (配置文件)
    
3. `my-daemon.service` (Systemd 启动脚本)
    

我们将不依赖任何外部构建工具（没有 Makefile，没有 CMake），直接在 Recipe 里掌控全局。

#### 完整的 `my-daemon_1.0.bb` 配方文件：

代码段

```shell
# ==========================================
# 1. 基础元数据区 (给菜谱建档)
# ==========================================
SUMMARY = "My custom C daemon for embedded system"
DESCRIPTION = "A background service that reads a config file and runs continuously."
LICENSE = "MIT"
# 因为是自己写的代码，没有单独的 LICENSE 文件，通常可以用通用的 MD5 绕过检查
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# ==========================================
# 2. 源码获取区 (准备食材)
# ==========================================
# file:// 告诉 Yocto 去当前 .bb 文件同级的 files/ 目录下找这三个文件
SRC_URI = " \
    file://main.c \
    file://my-daemon.conf \
    file://my-daemon.service \
"

# ==========================================
# 3. 继承高级魔法区 (调用预设模板)
# ==========================================
# 我们不需要编译模板，但我们需要 systemd 帮我们处理开机自启
inherit systemd

# 配合 systemd 模板的必填变量：指定服务文件的名字
SYSTEMD_SERVICE_${PN} = "my-daemon.service"
SYSTEMD_AUTO_ENABLE = "enable"

# ==========================================
# 4. 编译执行区 (重写 do_compile)
# ==========================================
# 因为没有 Makefile，我们亲自指挥交叉编译器
do_compile() {
    # ${CC} 包含了 arm-linux-gcc 及其基础架构参数
    # ${CFLAGS} 和 ${LDFLAGS} 包含了 Yocto 的安全编译选项
    # 文件默认在 ${WORKDIR} 下，我们直接编译并输出
    ${CC} ${CFLAGS} ${LDFLAGS} ${WORKDIR}/main.c -o ${WORKDIR}/my-daemon
}

# ==========================================
# 5. 装盘安装区 (重写 do_install，最容易出错的地方)
# ==========================================
do_install() {
    # 第一步：在虚拟盘子 ${D} 里把所有需要的抽屉（目录）建好
    install -d ${D}${bindir}                        # 对应 /usr/bin
    install -d ${D}${sysconfdir}                    # 对应 /etc
    install -d ${D}${systemd_system_unitdir}        # 对应 /lib/systemd/system

    # 第二步：把锅里（WORKDIR）的东西，精准地放进对应的抽屉里
    # 1. 安装可执行程序 (0755 权限：可读可写可执行)
    install -m 0755 ${WORKDIR}/my-daemon ${D}${bindir}/
    
    # 2. 安装配置文件 (0644 权限：只读，防篡改)
    install -m 0644 ${WORKDIR}/my-daemon.conf ${D}${sysconfdir}/
    
    # 3. 安装 Systemd 服务文件
    install -m 0644 ${WORKDIR}/my-daemon.service ${D}${systemd_system_unitdir}/
}
```

这就是一个工业级 Yocto Recipe 的本来面目。在这个文件中，你实现了代码的编译、配置文件的下发、以及系统级守护进程的注册。所有的“空间魔法”和“隐式行为”在这里都被显性化，形成了完整的闭环逻辑。