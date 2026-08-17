---
title: OSAL、库与应用框架模型
tags: [embedded, osal, adapter, observer, pubsub, active-object]
---

# OSAL、库与应用框架模型

返回总览：[[00-嵌入式代码模型学习地图]]

## 模型 18：OSAL 端口/适配器 + 不透明句柄

**优先级：P0｜层次：OSAL / Library**

### 第一性原理

可移植库需要“等待、互斥、线程、时间”，但 FreeRTOS、ThreadX、Zephyr 的类型、错误码、超时单位和 ISR 规则不同。直接散落条件编译会让平台差异渗透整个库。

将依赖反转：

- core 只依赖最小 port interface；
- 每个 OS 提供 adapter；
- 公共头只暴露不透明 handle 或由调用者提供的静态 storage；
- 统一单调时间、错误语义和 timeout；
- 能力不一致时选择最小公分母或显式 capability，不能假装语义相同。

### 费曼解释

旅行电器不需要理解每个国家的墙插，只依赖统一的适配器接口。适配器不仅改变插头形状，还必须保证电压和频率语义正确；只改函数名而不改语义会烧坏设备。

### 设计规则

| 问题 | 推荐契约 |
|---|---|
| 时间 | 单调 tick 或统一毫秒；提供 now 和 deadline/remaining |
| 无限等待 | 使用明确常量，不与最大有限值混淆 |
| 错误 | OK、TIMEOUT、WOULD_BLOCK、NO_MEMORY、INVALID 等稳定集合 |
| ISR | 单独的 ISR-safe API 或 capability |
| 内存 | 允许调用者提供 control block/storage，避免强制堆 |
| 句柄 | opaque pointer 或定长对齐 storage；不暴露底层 OS 类型 |

### 核心场景

~~~mermaid
sequenceDiagram
    participant L as "Portable Library"
    participant P as "OSAL Port Interface"
    participant A as "FreeRTOS/ThreadX/Zephyr Adapter"
    participant O as "Concrete OS"
    L->>P: mutex_lock(handle, deadline)
    P->>A: stable contract
    A->>O: native API + unit conversion
    O-->>A: native status
    A-->>L: OSAL_OK / OSAL_TIMEOUT
~~~

### 最小 Demo（C11，注入式端口）

~~~c
#include <stdint.h>
#include <stdio.h>

typedef enum { OSAL_OK, OSAL_TIMEOUT, OSAL_ERROR } osal_status_t;
typedef void *osal_mutex_t;

typedef struct {
    osal_status_t (*mutex_lock)(osal_mutex_t, uint32_t timeout_ms);
    void (*mutex_unlock)(osal_mutex_t);
    uint32_t (*now_ms)(void);
} osal_port_t;

typedef struct { const osal_port_t *port; osal_mutex_t lock; int value; } lib_t;

static osal_status_t mock_lock(osal_mutex_t m, uint32_t ms) {
    (void)m; (void)ms; return OSAL_OK;
}
static void mock_unlock(osal_mutex_t m) { (void)m; }
static uint32_t mock_now(void) { return 1234u; }

static int lib_increment(lib_t *lib) {
    if (lib->port->mutex_lock(lib->lock, 10u) != OSAL_OK) return -1;
    lib->value++;
    lib->port->mutex_unlock(lib->lock);
    return 0;
}

int main(void) {
    static const osal_port_t mock = { mock_lock, mock_unlock, mock_now };
    lib_t lib = { .port = &mock };
    lib_increment(&lib);
    printf("value=%d now=%u\n", lib.value, lib.port->now_ms());
}
~~~

### 何时不用与误用

- 应用只绑定一个 OS 且无独立库边界时，不必包住全部 RTOS；
- 不要创建一对一“改名层”却仍泄露 native type；
- mutex、binary semaphore、event flag 的语义不能互换；
- tick 到毫秒换算要处理截断、溢出和 forever；
- 适配层必须有 contract tests，在每个后端跑同一组测试。

### 参考实现

- [CMSIS-RTOS2：通用 RTOS API](https://arm-software.github.io/CMSIS_6/latest/RTOS2/index.html)
- [CMSIS-RTOS2 Message Queue](https://arm-software.github.io/CMSIS_6/latest/RTOS2/group__CMSIS__RTOS__Message.html)
- [Zephyr CMSIS-RTOS v2 compatibility](https://docs.zephyrproject.org/latest/services/portability/cmsis_rtos_v2.html)
- [FreeRTOS-Plus-TCP portable Network Interface](https://github.com/FreeRTOS/FreeRTOS-Plus-TCP/tree/main/source/portable/NetworkInterface)

### 本模型自检

- [x] 从平台差异泄漏推导端口/适配器
- [x] 语义表、Mermaid 与 mock Demo
- [x] 时间、ISR、内存和 contract test 边界
- [x] 官方 OSAL/兼容层 URL

---

## 模型 19：观察者 / 发布订阅

**优先级：P1｜层次：Library / User App**

### 第一性原理

生产者直接调用所有消费者会形成扇出依赖：新增日志、UI、遥测都要改生产者。把“事件是什么”与“谁关心它”分开，生产者只发布，订阅关系由框架维护。

必须区分两种语义：

- 同步 Observer：publish 在当前栈逐个调用，简单但会传播延迟和重入；
- 异步 Pub/Sub：发布到队列，隔离上下文但引入容量、复制/所有权和时序问题。

### 费曼解释

广播电台不逐个给听众打电话。它只按频道播出，听众自己订阅。同步 Observer 像会议室现场通知；异步 Pub/Sub 像邮箱，慢听众不会占住播音员，但邮箱可能塞满。

### 核心不变量

- 发布期间订阅表能否修改必须明确；
- 回调上下文和最大执行时间必须明确；
- 异步消息必须定义复制还是移交所有权；
- 满队列、慢订阅者和取消订阅竞态必须有策略；
- 关键命令不要用无确认的广播代替点对点请求。

### 核心场景

~~~mermaid
sequenceDiagram
    participant P as "Publisher"
    participant B as "Broker/Subject"
    participant U as "UI Subscriber"
    participant T as "Telemetry Subscriber"
    U->>B: subscribe(TEMP)
    T->>B: subscribe(TEMP)
    P->>B: publish(TEMP, 28)
    B-->>U: on_event
    B-->>T: on_event
~~~

### 最小 Demo（C11，同步快照式 Observer）

~~~c
#include <stddef.h>
#include <stdio.h>

#define MAX_SUBS 4
typedef void (*observer_fn)(int value, void *user);
typedef struct { observer_fn fn; void *user; } sub_t;
typedef struct { sub_t subs[MAX_SUBS]; size_t count; } subject_t;

static int subscribe(subject_t *s, observer_fn fn, void *user) {
    if (s->count == MAX_SUBS) return -1;
    s->subs[s->count++] = (sub_t){ fn, user };
    return 0;
}

static void publish(subject_t *s, int value) {
    size_t snapshot = s->count;
    for (size_t i = 0; i < snapshot; ++i) {
        s->subs[i].fn(value, s->subs[i].user);
    }
}

static void print_value(int value, void *user) {
    printf("%s=%d\n", (const char *)user, value);
}

int main(void) {
    subject_t s = {0};
    subscribe(&s, print_value, "ui");
    subscribe(&s, print_value, "telemetry");
    publish(&s, 28);
}
~~~

### 工程边界与误用

- 同步回调不得阻塞，最好禁止回调内直接修改订阅表；
- 异步广播会把一条消息变成 N 份容量压力；
- 事件名/ID 要版本化，payload 布局不能无声变化；
- 事件顺序若重要，需要单一 broker 或每主题序号；
- 需要返回值和强一致完成时，用 request/reply 或 command。

### 参考实现

- [LVGL Events](https://lvgl.io/docs/open/examples/event)
- [LVGL Observer](https://lvgl.io/docs/open/main-modules/observer/observer)
- [ETL Message Bus / Broker](https://www.etlcpp.com/docs/messaging/)
- [QP/C++ Publish-Subscribe API](https://www.state-machine.com/qpcpp/api.html)

### 本模型自检

- [x] 解释解耦目标和同步/异步差异
- [x] Mermaid 与有界订阅 Demo
- [x] 重入、慢订阅者、所有权和顺序边界
- [x] LVGL/ETL/QP 具体 URL

---

## 模型 20：主动对象 / 消息路由

**优先级：P2｜层次：User App Framework**

### 第一性原理

锁的根因是多个执行上下文共享可变状态。主动对象从根上改变所有权：每个模块独占自己的状态和事件队列，外部只能发送消息；对象在自己的上下文中逐个 run-to-completion 处理消息。

因此并发推理从“谁持有哪些锁”变成“哪些消息可能以什么顺序到达”。这不是免费午餐：队列容量、调度优先级、消息所有权和长 handler 仍需设计。

### 费曼解释

每个部门都有自己的档案室，别人不能进去改文件，只能递交申请单。部门按顺序处理申请并回信。这样不用在档案柜上挂很多锁，但申请单堆满和某个部门办事过慢仍会造成问题。

### 核心不变量

- 状态只被对象自己的 handler 写；
- 每个事件处理 run-to-completion；
- 对象之间只通过不可变消息或明确所有权移交交互；
- 队列有界、优先级和过载策略显式；
- 跨对象同步等待应避免，否则容易重建死锁环。

### 核心场景

~~~mermaid
sequenceDiagram
    participant S as "Sensor Active Object"
    participant Q as "Motor AO Queue"
    participant M as "Motor Active Object"
    S->>Q: post(SET_SPEED, 1200)
    Q-->>M: dequeue
    M->>M: 状态机处理消息
    M->>S: post(SPEED_REACHED)
    Note over S,M: 双方不直接修改对方状态
~~~

### 最小 Demo（C11，两个对象共享调度器）

~~~c
#include <stdbool.h>
#include <stdio.h>

typedef enum { MSG_SET, MSG_STOP } msg_id_t;
typedef struct { msg_id_t id; int value; } msg_t;
#define QN 4u

typedef struct {
    msg_t q[QN];
    unsigned rd, wr, used;
    int speed;
} motor_ao_t;

static bool post(motor_ao_t *ao, msg_t m) {
    if (ao->used == QN) return false;
    ao->q[ao->wr] = m;
    ao->wr = (ao->wr + 1u) % QN;
    ao->used++;
    return true;
}

static bool run_one(motor_ao_t *ao) {
    if (ao->used == 0u) return false;
    msg_t m = ao->q[ao->rd];
    ao->rd = (ao->rd + 1u) % QN;
    ao->used--;
    if (m.id == MSG_SET) ao->speed = m.value;
    if (m.id == MSG_STOP) ao->speed = 0;
    return true;
}

int main(void) {
    motor_ao_t motor = {0};
    post(&motor, (msg_t){ MSG_SET, 1200 });
    while (run_one(&motor)) {}
    printf("speed=%d\n", motor.speed);
}
~~~

### 何时不用与误用

- 极小系统只有一个简单循环时，多个主动对象会增加概念和队列成本；
- handler 内阻塞会破坏 run-to-completion；
- 消息中传裸指针却不写生命周期契约，会把共享状态偷偷带回来；
- 每个小对象一个线程会浪费栈，可让多个对象共享事件驱动内核；
- 同步 request/reply 链可能形成等待环。

### 参考实现

- [QP/C Active Objects 需求与语义](https://www.state-machine.com/qpc/srs-qp_ao.html)
- [QP/C 源码仓库](https://github.com/QuantumLeaps/qpc)
- [QP/C++ Conceptual Model](https://www.state-machine.com/qpcpp/conc-qp.html)
- [ETL Messages, Routers, Buses and FSMs](https://www.etlcpp.com/docs/tutorials/message-tutorial/)

### 本模型自检

- [x] 从共享状态根因推出状态独占
- [x] Mermaid 与最小主动对象 Demo
- [x] 队列、线程成本、指针消息和等待环边界
- [x] QP/ETL 官方 URL
