
### 1. 克隆master分支最新代码到发布路径
### 2.  执行命令 
```bash
# --no-local 是必须的，否则本地克隆会忽略深度限制
git clone "file://$(pwd)" --depth 6 --no-local ../release_project
```
### 3.  在源码root路径执行 
```
zip -r version.zip <path>
```

