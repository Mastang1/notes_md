|**优先级**|**缩写**|**英文全称**|**中文名称**|**核心功能与用途**|
|---|---|---|---|---|
|P0|COB-ID|Communication Object Identifier|通信对象标识符|11位CAN标识符（高4位功能码加低7位节点号），决定总线仲裁优先级与报文类型|
|P0|OD|Object Dictionary|对象字典|由16位索引与8位子索引寻址的参数键值表，连接底层通信栈与上层应用变量|
|P0|PDO|Process Data Object|过程数据对象|无协议头、无应答的实时数据传输报文，单帧最多装载8字节纯业务变量|
|P0|TPDO|Transmit PDO|发送过程数据对象|从站向主站或总线发送的实时过程数据报文（默认标识为0x180至0x480加节点号）|
|P0|RPDO|Receive PDO|接收过程数据对象|主站向从站下发的实时控制指令报文（默认标识为0x200至0x500加节点号）|
|P0|SDO|Service Data Object|服务数据对象|主从问答式可靠配置会话，按索引读写对象字典，支持加速、分段与块传输|
|P0|RSDO|Receive SDO|接收服务数据对象|SDO客户端（主站）向服务端（从站）发起的参数读写请求帧（0x600加从站号）|
|P0|TSDO|Transmit SDO|发送服务数据对象|SDO服务端（从站）对客户端（主站）读写请求的应答帧（0x580加从站号）|
|P0|NMT|Network Management|网络管理|主站控制从站状态机跃迁（初始化、预操作、操作、停止与复位）的最高优先级指令（0x000）|
|P1|SYNC|Synchronization Object|同步对象|主站周期广播的全网同步时钟节拍（0x080），触发多轴同步采样与同步执行|
|P1|EMCY|Emergency Object|紧急对象|从站发生硬件或应用故障时主动抢占发送的高优先级诊断告警报文（0x080加节点号）|
|P1|HB|Heartbeat|心跳报文|从站周期性广播自身当前NMT状态的保活报文（0x700加节点号），用于主站监测掉线|
|P1|CiA|CAN in Automation|国际CAN用户与制造商协会|负责制定、维护与认证CANopen及CAN相关工业标准的国际组织|
|P1|CiA 301|Application Layer and Communication Profile|应用层与通信规范|CANopen最核心的基础协议标准，定义OD、SDO、PDO、NMT等全部基础通信机制|
|P1|CiA 402|Drives and Motion Control Profile|驱动器与运动控制规范|伺服驱动器、步进电机与变频器行业子协议（定义0x6040控制字、0x6041状态字等）|
|P1|CiA 401|Generic I/O Modules Profile|通用输入输出模块规范|数字量与模拟量I/O采集模块的行业标准子协议（定义0x6000至0x6400系列索引）|
|P1|EDS|Electronic Data Sheet|电子数据表|采用INI格式描述从站对象字典结构与参数范围的模板文件，供主站配置工具导入|
|P1|CCS|Client Command Specifier|客户端命令指示符|SDO请求帧Byte 0的高3位，指示发起下载、发起上传、分段传输或异常中止|
|P1|SCS|Server Command Specifier|服务端命令指示符|SDO应答帧Byte 0的高3位，指示下载确认、上传应答、分段确认或异常中止|
|P2|COS|Change of State|状态改变触发|PDO异步事件传输机制（传输类型254或255），当映射变量值变化时立即触发发送|
|P2|DCF|Device Configuration File|设备配置文件|在EDS模板基础上固化了具体节点ID、波特率及实际参数值的单节点实例配置文件|
|P2|EEC|Emergency Error Code|紧急错误代码|EMCY紧急报文前2字节携带的16位标准故障分类码（如过流、过压、超温）|
|P2|LSS|Layer Setting Services|层设置服务|CiA 305标准，用于无硬件拨码开关的从站通过总线动态配置Node-ID与波特率|
|P2|TIME|Time Stamp Object|时间戳对象|主站向全网广播6字节绝对日历时间（0x100，含当日午夜毫秒数与自1984年天数）|
|P2|NG|Node Guarding|节点守护|早期的从站保活机制，由主站发送RTR远程帧轮询从站状态，现已被Heartbeat取代|
|P2|RTR|Remote Transmission Request|远程传输请求|CAN链路层远程帧标志位，CANopen中仅用于旧版节点守护或按需请求RTR型TPDO|
|P2|CoE|CANopen over EtherCAT|基于EtherCAT的CANopen|在工业以太网EtherCAT上直接复用CANopen对象字典、SDO邮箱与PDO映射的协议层|