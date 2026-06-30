_**该部分包含：
1.打包测试ko、elf等到test_image的解决方案；
2.区分做菜dao**_

### 📓 笔记 1：Yocto 临时测试包打包策略速查

针对“仅在测试阶段使用，严防污染生产固件”的需求，业界有以下 4 种分级策略：

|**方案层级**|**核心手段**|**适用场景**|**优缺点 / 风险点**|
|---|---|---|---|
|**一：无痕热推**|`devtool deploy-target`|纯网络环境调试（无需烧录固件）。|**最快。** 秒级推送验证。缺点是重启板子/未配置网络时无法使用。|
|**二：本地沙盒**|修改 `build/conf/local.conf`<br><br>  <br><br>`CORE_IMAGE_EXTRA_INSTALL += "xxx"`|必须烧录固件，且仅开发者自己本地测试。|**不污染 Git 代码库。** 缺点是容易遗忘，导致本地出的包和 CI 服务器出的包不一致。|
|**三：影子镜像**|新建 `core-image-test.bb`<br><br>  <br><br>`require core-image-minimal.bb`<br><br>  <br><br>`IMAGE_INSTALL += "xxx"`|团队级联调。既要打包含测试包的固件，又不想影响基础镜像。|**极其优雅。** 生产敲 `bitbake core-image-minimal`，测试敲 `bitbake core-image-test`。推荐！|
|**四：隔离测试层**|新建 `meta-test` 层。<br><br>  <br><br>在里面用 `image.bbappend` 附加测试包。|极度复杂的测试栈（包含特殊脚本、后门配置、修改密码等）。|**高阶架构方案。**<br><br>  <br><br>⚠️ **致命风险：** 必须使用 `IMAGE_BASENAME_append = "-test"` 改名，否则产物会与生产固件重名，导致产线错烧！|

### 🧠 笔记 2：彻底搞懂 `do_install` 与 `IMAGE_INSTALL` 的楚河汉界

为什么我的 Recipe 成功执行了 `do_install`，把 `.ko` 放进了虚拟盘子 `${D}`，但最后烧录系统时却找不到？

因为你混淆了 **“做菜（打包包）”** 和 **“配菜（装系统）”** 这两个完全独立的生命周期。

#### 1. 核心概念重塑

- **`do_install` (在 Recipe 层面发生)：**
    
    它的使命是告诉 Yocto：“如果有人需要我这个软件，我应该被安装在系统的什么位置。”
    
    它把文件放进 `${D}` 后，Yocto 会立刻把 `${D}` 里的东西**压缩成一个个独立的软件包**（比如 `.rpm`、`.ipk` 或 `.deb`），放到 Yocto 内部的“软件仓库”架子上。**这一步，菜做好了，装进打包盒了，但还没上桌。**
    
- **`IMAGE_INSTALL` (在 Image 层面发生)：**
    
    这是一份**终极购物清单（菜单）**。
    
    Yocto 最终组装 `rootfs`（根文件系统）时，根本不看你执行过多少个 `do_install`，它只看 `IMAGE_INSTALL` 这个列表里写了什么。清单上有的，它就去刚才的“软件仓库”架子上拿对应的 `.rpm` 包解压进去；清单上没有的，哪怕你 `do_install` 写出花来，这个包也只能永远躺在仓库架子上吃灰。
    

#### 2. 工作流边界推演图

想象 Yocto 是一条双线并行的流水线：

Plaintext

```
[第一条线：零件制造厂 (Recipe)]
源码 -> do_compile -> do_install (${D}) -> 打包系统 -> 生成 my-daemon.rpm (存放在 tmp/deploy/rpm)
源码 -> do_compile -> do_install (${D}) -> 打包系统 -> 生成 sample.ko.rpm (存放在 tmp/deploy/rpm)
源码 -> do_compile -> do_install (${D}) -> 打包系统 -> 生成 lib.ko.rpm    (存放在 tmp/deploy/rpm)
============================= 严格的物理边界 =============================
[第二条线：整车装配厂 (Image)]
读取 core-image-minimal.bb -> 发现 IMAGE_INSTALL = "my-daemon lib.ko" 
    -> 组装 rootfs -> (拿取 my-daemon.rpm 和 lib.ko.rpm 解压) -> 生成最终固件.wic
    ★ 结果：sample.ko 虽然被制造了，但落选了，不在固件里。
```

#### 3. 实战：如何构建带有不同组件的 Images？

搞懂了这个边界，构建差异化的系统镜像就变得像搭积木一样简单。这也是 Yocto 傲视所有传统构建工具的核心能力所在。

**场景需求：**

你们公司有一款硬件主板，但要卖给两个不同的客户。

- **客户 A（基础款）：** 只需要 `app-a` 和 `lib-base`。
    
- **客户 B（尊享款）：** 需要 `app-a`，还要附加的高级功能 `app-pro` 和一个特殊的驱动 `pro-drv.ko`。
    

**极度优雅的 Yocto 架构方案：**

**第一步：在底层无差别制造所有的“零件”（Recipe）**

你不必管客户需要什么，你只需要写好 `app-a.bb`, `lib-base.bb`, `app-pro.bb`, `pro-drv.bb`。确保每个配方内部的 `do_install` 都能正确把文件放进它们自己的 `${D}`。

**第二步：在顶层定义“菜单”（Image）**

不要用 `.bbappend` 去到处追加，而是利用**继承**创建不同的顶级镜像配方。

新建一个基础镜像，大家都需要的放这里：

代码段

```
# 位于: meta-mybsp/recipes-core/images/my-image-base.bb
inherit core-image
SUMMARY = "Base image for all products"

IMAGE_INSTALL = " \
    packagegroup-core-boot \
    app-a \
    lib-base \
"
```

为客户 A 创建镜像（可以直接复用基础）：

代码段

```
# 位于: meta-mybsp/recipes-core/images/my-image-customer-a.bb
require my-image-base.bb
SUMMARY = "Basic tier firmware for Customer A"
```

为客户 B 创建专属的高级镜像：

代码段

```
# 位于: meta-mybsp/recipes-core/images/my-image-customer-b.bb
require my-image-base.bb
SUMMARY = "Pro tier firmware for Customer B"

# 在基础款的基础上，只为客户 B 增加额外组件
IMAGE_INSTALL += " \
    app-pro \
    pro-drv \
"
```

**终极执行：**

- 今晚给客户 A 发版，你只需敲：`bitbake my-image-customer-a`。Yocto 只挑 A 需要的 `.rpm` 包。
    
- 明天给客户 B 发版，你只需敲：`bitbake my-image-customer-b`。Yocto 会把多出来的 `app-pro` 也打包进去。
    

**总结：**

`do_install` 是“我能提供什么”，`IMAGE_INSTALL` 是“系统最终要什么”。把它们分离，你就掌握了在一个代码库里，维护几十种不同配置硬件产品的终极密码。