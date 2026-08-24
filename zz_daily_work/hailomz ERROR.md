**问题描述**

在使用 `hailomz parse` 时遇到报错：`ImportError: cannot import name 'TorchInferenceModel'`。

  

**原因分析**

这是 Hailo Model Zoo (如 v2.18 版本) 的一个已知问题。官方代码库中意外保留了一个未被实际使用，却会引发版本导入冲突的冗余文件 `torch_infer.py`。

  

**解决方法**

直接删除该残留文件即可，无需重新编译环境。执行以下命令：

  

Bash

```
rm /home/niki/hailo-work/hailo_model_zoo/hailo_model_zoo/core/infer/torch_infer.py
```

_注：删除后再次运行原 `hailomz` 命令即可正常执行。_