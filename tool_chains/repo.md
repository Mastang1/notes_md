# Repo & Manifest 多工程管理速查指南

## 1. 核心设计思想

Repo 并不是用来取代 Git 的，而是建立在 Git 之上的**多仓库协同管理工具**，其核心思想如下：

1. **BOM (物料清单) 理念**：`manifest.xml` 就是整个超级工程的 BOM。Git 负责微观的单个模块历史，而 Repo 负责宏观记录“在特定时间点，哪些仓库的哪些特定版本，组合成了完整可用的系统”。
2. **模块解耦与权限隔离**：允许将系统（如 Yocto、AOSP）拆分为互不干扰的独立 Git 仓库（内核、BSP、应用、工具），各个团队在自己的仓库中迭代，最终通过 Manifest 统一集成。
3. **宏观版本控制**：整个项目的基线（Baseline）变迁不再是单个代码的提交，而是 **Manifest 仓库本身的版本迭代**。切换 Manifest 分支，即完成整个超级工程的状态切换。

---

## 2. 常用 Repo 命令速查

### 初始化与同步 (Init & Sync)
* **`repo init -u <URL>`**：在当前目录初始化 `.repo` 环境并拉取 manifest 仓库。
  * `-b <branch>`：指定 manifest 仓库的分支（默认 `master`）。
  * `-m <xml_file>`：指定使用的 manifest 清单文件（默认 `default.xml`）。
  * `--depth=1`：浅克隆 manifest 仓库，加快初始化速度。
  * `--reference=<dir>`：指定本地镜像目录，极大提升二次拉取速度。
* **`repo sync`**：根据 manifest.xml 的配置，同步所有远端代码到本地工作区。
  * `-j<N>`：开启 N 个并发线程（如 `-j8`）。
  * `-c` (current-branch)：仅拉取 manifest 中指定的当前分支，不拉取其他冗余分支。
  * `-d` (detach)：将所有项目强制切回到 manifest 中指定的 `revision`，丢弃本地的游离状态。
  * `--no-tags`：不拉取 tag，加速同步。

### 本地开发与批量操作 (Develop & Forall)
* **`repo start <branch_name> --all`**：基于 manifest 锁定的版本，在所有仓库（或指定仓库）新建本地开发分支并切换。
* **`repo forall -c "<command>"`**：遍历所有仓库，并在每个仓库的根目录执行指定的 shell 命令。
  * *示例*：`repo forall -c "git checkout develop"`（注意：如果某个仓库没有 develop 分支会报错，但 repo 会跳过并继续执行其他仓库）。
  * *示例*：`repo forall -c "git clean -dfx"`（清理所有仓库的未跟踪文件和编译产物）。
* **`repo status`**：查看整个工作区中，哪些仓库有未提交的本地修改。
* **`repo diff`**：查看所有仓库中未提交的代码 diff。

---

## 3. Manifest 编写与常用标签

`manifest.xml` 本质上是仓库映射与属性声明的集合。

```xml
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <!-- 1. remote: 定义远程代码服务器及其评审服务器 (如 Gerrit) -->
  <remote name="origin" fetch="ssh://[git.example.com/](https://git.example.com/)" review="ssh://[gerrit.example.com:29418/](https://gerrit.example.com:29418/)" />
  
  <!-- 2. default: 为未显式指定属性的 project 设置默认值 -->
  <default remote="origin" revision="refs/heads/master" sync-j="4" />

  <!-- 3. include: 模块化引入其他清单文件，解耦大型工程 -->
  <!-- <include name="bsp-layers.xml" /> -->

  <!-- 4. project: 核心标签，映射远程仓库到本地路径 -->
  <!-- 默认行为：使用 default 的 master 分支 -->
  <project name="platform/os/freertos" path="os/freertos" groups="core" />

  <!-- 覆盖分支：指定该仓库使用 develop 分支 -->
  <project name="platform/bsp/stm32" path="bsp/stm32" revision="refs/heads/develop" groups="bsp" />

  <!-- 锁定 Commit：直接锁定 Hash 值，确保上游更新不破坏本地构建 (常用于第三方组件) -->
  <project name="platform/drivers/ipc-shm" path="drivers/ipc" revision="a1b2c3d4e5f67890123456789abcdef012345678" />

  <!-- 5. copyfile / linkfile: 同步后，将仓库内文件拷贝或软链接到超级工程的根目录 -->
  <project name="platform/build-tools" path="build" groups="tools">
    <copyfile src="Makefile" dest="Makefile" />
    <linkfile src="scripts/setup_env.sh" dest="setup_env.sh" />
  </project>
</manifest>
```

---

## 4. 变体 (Variant) 与版本 (Version) 管理官方推荐设计

在管理多个产品变体或迭代版本时，遵循**“Manifest 分支结构与物理代码时间线同构”**的原则。

### 场景一：独立的演进路线 / 不同的产品线（强烈推荐多 Branch）
如果变体 A 和 变体 B 已经是独立演进的产品线（如 `android-10` 和 `android-11`，或者定制化差异极大的两款 SoC Yocto 工程）：
* **设计策略**：在 Manifest 仓库中创建**不同的 Branch**（如 `branch_A` 和 `branch_B`）。
* **XML 命名**：两个分支内部统一使用 `default.xml`。
* **优势**：
  1. 避免 Git Log 污染：变体 A 的修改历史不会出现在变体 B 的分支里。
  2. 规避“文件名地狱”：`<include>` 的子文件不需要重命名（无需 `bsp-a.xml`），文件结构清爽。
  3. 天然适配 CI/CD：Gerrit/Jenkins 等工具天生对 Branch push 事件支持最佳。

### 场景二：相同的基线，不同的构建目标（推荐单 Branch 多 XML）
如果变体基于**完全相同的代码时间线（同一基线）**，仅仅是拉取的模块范围不同或针对不同的调试场景：
* **设计策略**：在 Manifest 仓库的**同一个 Branch**（如 `master`）中放置多个 XML。
* **XML 命名**：使用描述性命名，如 `default.xml` (全量), `minimal.xml` (最小化系统), `ci-test.xml` (仅包含测试框架)。
* **使用方式**：通过 `repo init -m minimal.xml` 来按需拉取。

### 场景三：多变体中存在的“完全共享”底层仓库
如果工程中的某些外部仓库（如 Yocto 的 `meta-openembedded`，或通用的 `common-bsp`）对变体 A 和 B 是**一模一样**的：
* **设计策略**：
  1. 外部仓库：**坚决只保留一个公共分支**（如 `master`），千万不要为了形式统一去生造冗余的 `branch_A`。这遵循了 DRY（Don't Repeat Yourself）和延迟拉分支（Branch-Late）的最佳实践。
  2. Manifest 仓库：在 Manifest 仓库的 `branch_A` 和 `branch_B` 的 `default.xml` 中，将该公共仓库的 `revision` **共同指向外部的 `master` 分支或特定的 Commit Hash**。
  3. 修改公共代码只需提交、Review 一次，所有变体即可自动受益。