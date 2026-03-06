这种自动转换**只针对基础数据类型（Simple data types）**。

- `c_int`, `c_short`, `c_long` -> Python `int`
    
- `c_float`, `c_double` -> Python `float`
    
- `c_char`, `c_char_p` -> Python `bytes` (或通过 `decode` 转为 `str`)
    
- `c_bool` -> Python `bool`