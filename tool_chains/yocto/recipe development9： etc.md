## 1. external build后clean local objects 测试这个命令有效
```shell
do_install_append(){
bbnote "hello"
cd ${B}
oe_runmake clean
}
```

## 2. todo
