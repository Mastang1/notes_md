## 1.  上层统一API
 - uniq_register()
 - uniq_open(enum e_uart/e_can);return struct CAN 指针 p_dev;
 - uniq_write(p_dev)
 - uniq_read(p_dev)
 - uniq_close(p_dev)
 - uniq_ctrl(p_dev)
 不同外设，封装为structure UART/structure CAN等,包含具体的读写关闭/控制等具体函数接口，具体实现根据不同HAL进行实现；
 uniq_open(enum e_uart/e_can）通过枚举类型和结构体静态变量的映射，返回对应的指针，后续读写操作直接调用该指针对应的读写操作；
