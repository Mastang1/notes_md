## 1. sysroot 机制
_**1. 一些初步的信息：
 - 1 存在一个集中存储依赖文件的路径 _**build/tmp/sysroot_components**_,当 lib-module 执行完 do_install 后，Yocto 的 *do_populate_sysroot 任务*会被触发。这个任务会把 ${D} 中属于开发类的目录（如 /usr/include, /usr/lib）复制或硬链接到这个中央仓库的对应子目录下。【本质就是一个仓库】；
 - 2 执行build的过程中，各个组件的构建的外部依赖，指向路径_**<对应模块的WORKDIR>/recipe-sysroot**_ ,就是临时代码路径下的一个名为recipe-sysroot的子路径；它是当前模块编译时的真实隔离环境（即 STAGING_INCDIR 所在的根目录）。编译器运行时的 sysroot 参数指向的就是这里。这里面只有你显式声明过依赖的东西。除了 recipe-sysroot (用于存放目标板架构的依赖)，你可能还会看到 recipe-sysroot-native，那里面存放的是在 Host 开发机上运行的工具（比如交叉编译器、cmake 等）。
 -
 - 3. sysroot_components  和 recipe_sysroot之间的关系：
 - 4. sysroot_componts 如何操作（recipe中写啥），才会存储指定的headers、symbols到该路径？