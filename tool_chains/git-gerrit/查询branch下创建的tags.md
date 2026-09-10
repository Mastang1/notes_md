**Git 中的 Tag 是全局的（仓库级别的），与分支没有分组或包含关系。**

##  git tag --merged <branch-name>

### 1. 本质：Tag 指向的是 Commit，而不是分支

- **分支（Branch）**：是一个会随着新的提交（Commit）不断向前移动的指针（存放在 `refs/heads/`）。
    
      
    
- **Tag**：是一个固定不变的指针，直接绑定在一个具体的 **Commit Hash** 上（存放在 `refs/tags/`）。
    
      
    

当你在 `main` 分支上执行 `git tag v1.0` 时，Git 只是将 `v1.0` 贴在了**当前 HEAD 所指向的那个 Commit 上**，并没有记录“这个 Tag 属于 main 分支”的信息。

  

### 2. 独立性的具体表现

- **跨分支可见**：无论你切换到哪个分支，运行 `git tag` 看到的都是整个仓库全局的 Tag 列表。
    
      
    
- **删除分支不影响 Tag**：即使你删除了创建 Tag 时的那个分支，Tag 以及它所指向的 Commit 依然会完好无损地保留在仓库中。
    
      
    
- **远程仓库共享**：推送 Tag（`git push origin v1.0`）时，Tag 会存放在远程的全局 `refs/tags/` 空间下，所有拉取该仓库的人都能看到。
    
      
    

### 3. 如何查询 Tag 与分支的关系

虽然 Tag 在结构上不属于任何分支，但可以通过提交历史来查询它们在拓扑图上的交集：

  

- **查看某个分支的历史中包含了哪些 Tag：**
    
      
    
    Bash
    
    ```
    git tag --merged <branch-name>
    ```
    
- **查看包含某个 Tag 的所有分支：**
    
    Bash
    
    ```
    git branch --contains <tag-name>
    ```