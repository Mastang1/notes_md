## 1. external build后clean local objects
```shell
do_install_append(){
bbnote "hello"
cd ${B}
oe_runmake clean
}
```

## 2. todo
