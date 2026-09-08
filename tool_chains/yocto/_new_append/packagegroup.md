代码段

```c
SUMMARY = "Hailo RT requirements"
DESCRIPTION = "The set of packages required to enable Hailo RT functionality"

PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit packagegroup

PACKAGEGROUP_DISABLE_COMPLEMENTARY = "1"
PACKAGES_BY_FEATURES = "\
    ${@bb.utils.contains('DISTRO_FEATURES', 'hrt-gen-ai', '${PN}-gen-ai', '', d)} \
    ${@bb.utils.contains('DISTRO_FEATURES', 'hrt-server', '${PN}-pci-server', '', d)} \
    "
PACKAGES = "${PN} \
            ${PACKAGES_BY_FEATURES} \
            "

RDEPENDS_${PN} = "\
    hailortcli \
    libhailort \
    ${PACKAGES_BY_FEATURES} \
    "

PCI_SERVER_PACKAGES = ""
RDEPENDS_${PN}-pci-server = "${PCI_SERVER_PACKAGES}"

GEN_AI_PACKAGES = "pyhailort"
RDEPENDS_${PN}-gen-ai = "${GEN_AI_PACKAGES}"
```

## 核心机制：主包、子包与依赖链

### 1. 包的关系与角色

- **主包 (`${PN}`)**：即 `packagegroup-hailo-hailort`。作为主入口，它**强制绑定**基础必选依赖（`hailortcli` 和 `libhailort`）。
    
      
    
- **子包 (`${PN}-<feature>`)**：即 `packagegroup-hailo-hailort-gen-ai` 等。它们是**条件触发的“代理包”**，不包含实际二进制，仅声明子功能依赖（如 `pyhailort`）。
    
      
    
- **动态控制**：`PACKAGES_BY_FEATURES` 相当于条件开关。只有开启对应的 `DISTRO_FEATURES`，子包名才会被注入到 `PACKAGES`（注册包名）和 `RDEPENDS_${PN}`（主包依赖列表）中。
    
      
    

### 2. 运行时依赖传递链（以开启 `hrt-gen-ai` 为例）

```
IMAGE_INSTALL += "packagegroup-hailo-hailort"
  └──> 主包: packagegroup-hailo-hailort
        ├──> 强制依赖: hailortcli (可执行文件)
        ├──> 强制依赖: libhailort (动态库)
        └──> 条件依赖: packagegroup-hailo-hailort-gen-ai (子包)
              └──> 依赖: pyhailort (Python绑定) ──> 拉取 python3 基础环境
```

## 构建全流程与底层物理路径

假设工程构建目录为 `build/`，目标单板为 `imx8mq`，包格式为 `ipk`（RPM/DEB 逻辑相同）。

  

### 阶段一：Recipe 产生包（`do_package_write_ipk`）

当执行 `bitbake packagegroup-hailo-hailort` 时：

  

1. **编译工作区（Work Directory）**：
    
    `build/tmp/work/imx8mq-poky-linux/packagegroup-hailo-hailort/1.0-r0/`
    
    BitBake 在此完成变量解析和包控制文件（`control`）生成。
    
      
    
2. **生成独立包文件**：
    
    根据 `PACKAGES` 变量解析结果，BitBake 会在打包目录下生成对应的空元数据包：
    
      
    - `build/tmp/deploy/ipk/imx8mq/packagegroup-hailo-hailort_1.0-r0_imx8mq.ipk`
        
          
        
    - `build/tmp/deploy/ipk/imx8mq/packagegroup-hailo-hailort-gen-ai_1.0-r0_imx8mq.ipk`（若特性开启）
        
          
        
    - _注：此时 `hailortcli`、`libhailort`、`pyhailort` 已由各自对应的 Recipe 生成并存放在 `build/tmp/deploy/ipk/` 中。_
        
          
        

### 阶段二：镜像组装根文件系统（`do_rootfs`）

当执行 `bitbake core-image-minimal` 时：

  

1. **创建索引（Index）**：
    
    包管理器（如 `opkg`）扫描包输出目录 `build/tmp/deploy/ipk/`，生成局域包 index 索引。
    
      
    
2. **拉取与依赖求解（Dependency Resolution）**：
    
    包管理器根据 `IMAGE_INSTALL` 找到 `packagegroup-hailo-hailort_1.0-r0_imx8mq.ipk`，读取其 `control` 文件中的 `Depends:` 字段，递归解析出所有关联包：
    
    `packagegroup-hailo-hailort` ➔ `hailortcli` + `libhailort` + `packagegroup-hailo-hailort-gen-ai` ➔ `pyhailort`。
    
      
    
3. **解压解包至根文件系统**：
    
    包管理器从 `build/tmp/deploy/ipk/` 提取所有涉及到的 `.ipk` 包，**直接解压并填充到镜像临时 Rootfs 目录**：
    
    `build/tmp/work/imx8mq-poky-linux/core-image-minimal/1.0-r0/rootfs/`
    
      
    
4. **打包最终 Image**：
    
    将 `rootfs/` 目录压制为最终的镜像文件（如 `.wic` 或 `.ext4`），输出至：
    
    `build/tmp/deploy/images/imx8mq/`