---
title: HAL 与驱动框架模型
tags: [embedded, hal, driver, vtable, registry, async-io, intrusive-list]
---

# HAL 与驱动框架模型

返回总览：[[00-嵌入式代码模型学习地图]]

## 模型 14：ops 函数表 + 实例上下文

**优先级：P0｜层次：HAL / Driver Framework**

### 第一性原理

业务需要的是“发送、接收、控制”，硬件提供的是不同寄存器和 IRQ。若业务直接调用芯片 HAL，变化会沿依赖链向上传播。

C 中最小的动态多态结构是：

- 一张只读 ops 表描述“这种实现能做什么”；
- 一个 context 指针描述“这个实例的数据在哪里”；
- 对外 handle 把二者绑定；
- config 存不变硬件描述，data 存运行时可变状态。

调用者只依赖契约，STM32、Nordic、仿真实现可以互换。

### 费曼解释

遥控器按钮是统一接口，电视机内部实现不同。ops 是按钮说明书，context 是“具体控制客厅哪一台电视”。只给按钮没有实例，就不知道控制谁；只给实例没有 ops，就不知道怎么控制。

### 核心不变量

- ops 表在对象生命周期内有效，通常放只读存储；
- context 类型只由具体驱动解释；
- 每个入口检查 handle、能力和生命周期状态；
- ISR 回调的上下文、可重入性和缓冲所有权属于 API 契约；
- 通用 API 不暴露芯片寄存器类型。

### 核心场景

~~~mermaid
sequenceDiagram
    participant A as "Middleware"
    participant H as "driver_handle"
    participant O as "const ops"
    participant C as "instance context"
    A->>H: send(data, len)
    H->>O: ops->send(ctx, data, len)
    O->>C: 访问该实例寄存器/状态
    C-->>A: result
~~~

### 最小 Demo（C11）

~~~c
#include <stddef.h>
#include <stdio.h>

typedef struct {
    int (*write)(void *ctx, const void *data, size_t len);
    int (*set_baud)(void *ctx, unsigned baud);
} serial_ops_t;

typedef struct {
    const serial_ops_t *ops;
    void *ctx;
} serial_t;

typedef struct { const char *name; unsigned baud; } mock_uart_t;

static int mock_write(void *ctx, const void *data, size_t len) {
    mock_uart_t *u = ctx;
    printf("%s writes %zu bytes at %u baud\n", u->name, len, u->baud);
    (void)data;
    return 0;
}

static int mock_set_baud(void *ctx, unsigned baud) {
    ((mock_uart_t *)ctx)->baud = baud;
    return 0;
}

static const serial_ops_t mock_ops = {
    .write = mock_write,
    .set_baud = mock_set_baud,
};

int main(void) {
    mock_uart_t impl = { .name = "UART0", .baud = 9600u };
    serial_t uart = { .ops = &mock_ops, .ctx = &impl };
    uart.ops->set_baud(uart.ctx, 115200u);
    return uart.ops->write(uart.ctx, "OK", 2u);
}
~~~

### 何时不用与误用

- 只有一个实现且永不测试替换时，直接函数可能更清楚；
- 不要为每个寄存器操作都做虚调用，抽象应对应稳定能力；
- 不要把所有设备塞进万能 ops，按接口类别拆分；
- 能力可选时提供 capabilities，避免空函数指针猜测；
- 版本化公共 ABI，函数表顺序不能随意变化。

### 参考实现

- [CMSIS-Driver Theory of Operation：Access Struct 与实例](https://arm-software.github.io/CMSIS_6/latest/Driver/theoryOperation.html)
- [CMSIS-Driver SPI Access Struct](https://arm-software.github.io/CMSIS_6/latest/Driver/Driver__SPI_8h.html)
- [Zephyr Device Driver Model](https://docs.zephyrproject.org/latest/kernel/drivers/index.html)
- [CMSIS-Driver 实现仓库](https://github.com/ARM-software/CMSIS-Driver)

### 本模型自检

- [x] 从变化隔离推导 ops + context
- [x] Mermaid 与可替换 mock Demo
- [x] config/data、能力、ABI 边界
- [x] CMSIS/Zephyr 官方 URL

---

## 模型 15：静态注册表 + 分阶段初始化

**优先级：P1｜层次：Driver Framework / Library**

### 第一性原理

大量驱动或命令若都手写进中心数组，会制造一个频繁冲突的修改点。链接器本来就会收集各编译单元的静态对象，因此可把同类型描述符放进专用 section，在启动时遍历。

但“发现对象”和“按依赖安全初始化”是两个问题。初始化必须有显式阶段或依赖图：时钟先于总线，总线先于设备，OS 服务只能在内核就绪后使用。

### 费曼解释

参展商各自在门口放一张登记卡，开馆时主办方沿着一排卡片清点，不需要每次新增参展商都重写总名单。但进场顺序仍要管：先通电，再搭展台，最后开放参观。

### 核心场景

~~~mermaid
sequenceDiagram
    participant L as "Linker"
    participant B as "Boot"
    participant R as "Registry Section"
    participant D as "Driver"
    L->>R: 收集各模块描述符
    B->>R: iterate(PRE_KERNEL)
    R->>D: init(clock)
    B->>R: iterate(POST_KERNEL)
    R->>D: init(bus/device)
~~~

### 最小 Demo（C11，可移植显式 section 边界的简化版）

~~~c
#include <stddef.h>
#include <stdio.h>

typedef int (*init_fn)(void);
typedef struct { const char *name; unsigned level; init_fn init; } entry_t;

static int clock_init(void) { puts("clock"); return 0; }
static int uart_init(void)  { puts("uart");  return 0; }

static const entry_t registry[] = {
    { "clock", 0u, clock_init },
    { "uart",  1u, uart_init  },
};

static int init_level(unsigned level) {
    for (size_t i = 0; i < sizeof registry / sizeof registry[0]; ++i) {
        if (registry[i].level == level && registry[i].init() != 0) {
            printf("init failed: %s\n", registry[i].name);
            return -1;
        }
    }
    return 0;
}

int main(void) {
    if (init_level(0u) != 0) return 1;
    return init_level(1u);
}
~~~

### 工程边界与误用

- section 名、对齐、KEEP 和起止符号依赖工具链，需链接脚本测试；
- 链接垃圾回收可能删除“无人引用”的注册项；
- 相同 level 内不能靠文件链接顺序表达依赖；
- 初始化失败必须传播，调用者还需检查 device ready；
- 安全项目可选择显式生成表，以获得更强的可追溯性。

### 参考实现

- [Zephyr Iterable Sections](https://docs.zephyrproject.org/latest/kernel/iterable_sections/index.html)
- [Zephyr Device Driver Model 与初始化级别](https://docs.zephyrproject.org/latest/kernel/drivers/index.html)
- [Zephyr Devicetree HOWTO：DEVICE_DT_GET 与 ready 检查](https://docs.zephyrproject.org/latest/build/dts/howtos.html)
- [Linux initcall 级别源码](https://github.com/torvalds/linux/blob/master/include/linux/init.h)

### 本模型自检

- [x] 区分发现与依赖初始化
- [x] Mermaid 与分阶段 Demo
- [x] 链接器 GC、顺序和失败边界
- [x] 官方文档/源码 URL

---

## 模型 16：异步命令 + 完成事件

**优先级：P1｜层次：HAL / Driver / OS**

### 第一性原理

DMA、SPI、Flash 等操作的开始和完成分属不同时间点。若同步等待，调用线程被占用；若只返回成功，又无法表达稍后的结果。

最小异步契约包含：

- start：验证参数、占有请求、启动硬件；
- completion：由 IRQ/DMA 记录最终状态；
- notify：把完成事件交到允许的上下文；
- token/generation：区分旧完成、取消和新请求；
- ownership：规定完成前缓冲不可复用。

### 费曼解释

送洗衣店不是站在柜台等两个小时。店员给你一张带号码的票，洗完通知你。票号防止把上一次订单的完成消息认成这一次。

### 核心场景

~~~mermaid
sequenceDiagram
    participant A as "Application"
    participant D as "Driver"
    participant HW as "DMA/Peripheral"
    participant Q as "Completion Queue"
    A->>D: start(req, token=7)
    D->>HW: program + enable
    D-->>A: IN_PROGRESS
    HW->>D: IRQ complete
    D->>Q: post(DONE, token=7, status)
    Q-->>A: completion
    A->>A: release buffer / next state
~~~

### 最小 Demo（C11）

~~~c
#include <stdbool.h>
#include <stdio.h>

typedef void (*done_fn)(unsigned token, int status, void *user);

typedef struct {
    bool busy;
    unsigned generation;
    done_fn done;
    void *user;
} async_driver_t;

static int start(async_driver_t *d, done_fn cb, void *user,
                 unsigned *token) {
    if (d->busy) return -1;
    d->busy = true;
    d->done = cb;
    d->user = user;
    *token = ++d->generation;
    return 0; /* 实机在这里启动 DMA */
}

static void completion_isr(async_driver_t *d, unsigned hw_token, int status) {
    if (!d->busy || hw_token != d->generation) return;
    d->busy = false;
    d->done(hw_token, status, d->user);
}

static void on_done(unsigned token, int status, void *user) {
    (void)user;
    printf("token=%u status=%d\n", token, status);
}

int main(void) {
    async_driver_t d = {0};
    unsigned token;
    start(&d, on_done, NULL, &token);
    completion_isr(&d, token, 0);
}
~~~

### 工程边界与误用

- 示例直接在 ISR 回调只为展示骨架；生产代码要明确回调上下文，复杂回调应延后；
- 取消不是简单清 busy，必须与飞行中的 IRQ/DMA 竞态协商；
- token 要防回绕误认，或结合 busy/epoch；
- start 返回成功只表示“已接受”，不表示传输成功；
- 超时后硬件仍可能完成，必须处理 late completion。

### 参考实现

- [CMSIS-Driver Theory：非阻塞传输与 SignalEvent](https://arm-software.github.io/CMSIS_6/latest/Driver/theoryOperation.html)
- [CMSIS USART Driver](https://arm-software.github.io/CMSIS_6/latest/Driver/group__usart__interface__gr.html)
- [Zephyr UART Asynchronous API](https://docs.zephyrproject.org/latest/hardware/peripherals/uart.html#asynchronous-api)
- [Linux completions](https://docs.kernel.org/scheduler/completion.html)

### 本模型自检

- [x] 解释开始与完成的时间分离
- [x] Mermaid 含 token 和完成队列
- [x] Demo 含 generation 防旧完成
- [x] 取消、超时、late completion 和上下文边界
- [x] 官方驱动 URL

---

## 模型 17：侵入式容器

**优先级：P2｜层次：OS / Driver Framework / Library**

### 第一性原理

通用容器通常另外分配 node，并保存指向 payload 的指针。在内存受限、对象生命周期受控的系统里，这带来额外分配和一次间接访问。

若把链表节点嵌入业务对象，对象本身就是容器节点：

- 插入/删除无需分配；
- 一个对象可嵌入多个不同 link，加入多个索引；
- 通过成员偏移从 link 恢复宿主对象。

代价是容器关系进入对象布局，生命周期和“是否已入链”必须严格管理。

### 费曼解释

普通寄存处给每件行李另做一张吊牌；侵入式容器让行李箱自带标准挂环。传送带直接勾挂环，不需要临时做牌，但你必须知道哪个挂环属于哪条传送带。

### 核心场景

~~~mermaid
sequenceDiagram
    participant O as "Object"
    participant L as "Embedded Link"
    participant Q as "Intrusive List"
    O->>L: 对象内自带节点
    Q->>L: insert/remove
    Q->>L: iterate node
    L-->>Q: container_of(node) -> Object
~~~

### 最小 Demo（C11）

~~~c
#include <stddef.h>
#include <stdio.h>

typedef struct link { struct link *next; } link_t;
typedef struct {
    int id;
    link_t ready_link;
} job_t;

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

static void push(link_t **head, link_t *node) {
    node->next = *head;
    *head = node;
}

int main(void) {
    job_t a = { .id = 1 }, b = { .id = 2 };
    link_t *ready = NULL;
    push(&ready, &a.ready_link);
    push(&ready, &b.ready_link);

    for (link_t *n = ready; n != NULL; n = n->next) {
        job_t *j = CONTAINER_OF(n, job_t, ready_link);
        printf("job=%d\n", j->id);
    }
}
~~~

### 工程边界与误用

- 对象释放前必须从所有容器移除；
- 同一个 link 不能同时加入两个链表；
- 多链归属要用不同成员名；
- 指针损坏会污染容器，调试版应加 poison/owner 标记；
- API 边界若需要封装稳定性，非侵入式容器可能更合适。

### 参考实现

- [Linux Linked Lists](https://docs.kernel.org/core-api/list.html)
- [Linux list.h 源码](https://github.com/torvalds/linux/blob/master/include/linux/list.h)
- [Zephyr Doubly-linked List](https://docs.zephyrproject.org/latest/kernel/data_structures/dlist.html)
- [Zephyr SLists](https://docs.zephyrproject.org/latest/kernel/data_structures/slist.html)

### 本模型自检

- [x] 从额外分配成本推出嵌入节点
- [x] Mermaid 与 container-of Demo
- [x] 生命周期、多链归属、损坏边界
- [x] Linux/Zephyr 官方 URL

