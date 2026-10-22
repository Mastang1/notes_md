>核心就是 BOOT/UPDATE分区的state判断，有代表性的组合是 BOOT:testing;UPDATE:updating;
>1. 断电续传；2. boot是否为testing，决定是否回退fallback；3. update是否为updating，决定是否swap并升级。
>![[Pasted image 20261021000747.png]]