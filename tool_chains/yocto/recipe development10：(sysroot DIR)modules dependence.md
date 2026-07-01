## 1. sysroot 机制
_**1. 一些初步的信息：
 - 1 存在一个集中存储依赖文件的路径 _**build/tmp/sysroot_components**_,当 lib-module 执行完 do_install 后，Yocto 的 *do_populate_sysroot 任务*会被触发。这个任务会把 ${D} 中属于开发类的目录（如 /usr/include, /usr/lib）复制或硬链接到这个中央仓库的对应子目录下。【本质就是一个仓库】；_**亦即：s1: moduled build完成，执行install s2：自动触发do_populate_sysroot()流程，所以前提是s1先拷贝需要的文件到 ${D}**_
 - 2 _**每个组件执行build的过程中, 各自依赖workdir下的 recipe-sysroot, 并且，只有recipe中依赖指定组件时候(如：DEPENDS = "lib-module")，才会执行从公共sys-root拷贝指定组件文件到当前recipe-sysroot**_，各个组件的构建的外部依赖，指向路径_**<对应模块的WORKDIR>/recipe-sysroot**_ ,就是临时代码路径下的一个名为recipe-sysroot的子路径；它是当前模块编译时的真实隔离环境（即 STAGING_INCDIR 所在的根目录）。编译器运行时的 sysroot 参数指向的就是这里。这里面只有你显式声明过依赖的东西。除了 recipe-sysroot (用于存放目标板架构的依赖)，你可能还会看到 _**recipe-sysroot-native，那里面存放的是在 Host 开发机上运行的工具（比如交叉编译器、cmake 等）。
 -
 - **3. sysroot_componts 如何操作（recipe中写啥），才会存储指定的headers、symbols到该路径？**
>	 - symbols: MODULES_MODULE_SYMVERS_LOCATION = "."
>原理机制：
>### 自动导出的幕后功臣：`module.bbclass`

在你的 Recipe 第 5 行，写了 `inherit module`。 Yocto 的 `module.bbclass` （处理内核模块的核心类）其实内部早就考虑到“模块间相互依赖”的场景了。为了方便大家，它内置了一段自动打包符号表的逻辑，伪代码类似这样：

1. 它会去读一个特殊变量：`MODULES_MODULE_SYMVERS_LOCATION`。
    
2. 如果你在 Recipe 里定义了这个变量（比如你定义了 `.`，代表当前编译根目录），`module.bbclass` 内置的默认 `do_install` 就会在底层替你执行类似下面的操作： `install -m 0644 ${B}/./Module.symvers ${D}/lib/modules/${KERNEL_VERSION}/extra/`
- 4 app module，

