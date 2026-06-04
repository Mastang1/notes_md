##### 当前C1的启动链路💚
 - 1. 打包阶段：通过blob-tool打包 DCD、BSE firmware、IVT、first_app(BootLoader.bin)生成blob.bin
 - 2. 应用打包阶段：使用app_packer工具，打包blob.bin 及soc几个核心的bin文件，大概是将启动地址放到了bootloader.bin的指定偏移地址，生成mip_bl_app.bin
 - 3. 串行启动(flashtool show time)：HSE核心启动、接收flash_loader.bin到ram中；BSE core操作启动cm7_0,跳转到flash_loader.bin执行；进入到flash_tool交互模式，通过发送 命令及参数开启文件传输及数据存储(qspi/sd/emmc)；注意：flash_loader在ram中。（一次串行启动完成所有传输）
 - 4. QSPI启动模式启动：HSE启动，执行boot流程，查询IVT，HES core执行HSE firmware，CM7_0执行bootloader代码，bootloader根据固定便宜位置的app地址及跳转地址执行顺序加载多核运行；