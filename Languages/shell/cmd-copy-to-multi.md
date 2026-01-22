
```shell
# 原理：echo列出所有目录 -> 管道传给xargs -> xargs把目录拆分并传给cp的-t参数
echo ./c1* | xargs -n 1 cp a.c -t
```