为了为您提供一个更加清晰、结构化的说明，我们将严格按照您的要求，从**核心 API 与调用流程 Demo**、**核心类**、**UML 序列图**以及**线程模型与关系**四个方面，为您深度梳理 `sherpa-onnx` 中的关键词识别（KWS）架构。

_注：提供的参考资料,, 中主要涵盖了 `sherpa-onnx` 支持多种编程语言（C/C++等）、多平台（如 RK NPU, Mac, Linux 等）以及底层基于 onnxruntime 的高层架构信息。以下具体的类名、API 与线程设计细节基于 `sherpa-onnx` 开源仓库的标准工程实现补充说明。_

---

### 一、 KWS 核心 API 与实际场景调用流程 Demo

在工程部署中，关键词识别（KWS）被设计为流式（Streaming）处理。以下是基于 C++ API 封装的核心接口抽象及其实际场景调用伪代码：

#### 1. 核心 API 概览

- `CreateKeywordSpotter(config)`：初始化引擎，加载 ONNX 模型和唤醒词词典。
- `CreateStream()`：为音频通道创建一个独立的上下文数据流。
- `AcceptWaveform(sample_rate, pcm_data)`：接收底层硬件（如麦克风）传来的实时音频 PCM 数据。
- `IsReady()`：检查流中是否已积累足够帧数的音频特征，满足模型推理所需的最小数据量。
- `Decode()`：执行单次 ONNX 模型的前向推理和图搜索。
- `GetResult()`：获取识别结果（返回唤醒词字符串及时间戳）。
- `Reset()`：一旦命中关键词或句子结束，重置流状态以准备下一次唤醒。

#### 2. 实际场景调用流程 Demo（C++ 风格伪代码）

```c++
#include "sherpa-onnx/cplusplus/keyword_spotter.h"

// 实际场景：智能音箱的实时唤醒流程
void RunKeywordSpottingDemo() {
    // 1. 配置并初始化 KWS 引擎 (只执行一次)
    sherpa_onnx::KeywordSpotterConfig config;
    config.model_config.encoder = "zipformer-encoder.onnx";
    config.model_config.decoder = "zipformer-decoder.onnx";
    config.keywords_file = "keywords.txt"; // 例如包含 "xiao zhi xiao zhi"

    auto spotter = sherpa_onnx::KeywordSpotter::Create(config);

    // 2. 创建音频数据流 (每路麦克风一个)
    auto stream = spotter->CreateStream();

    // 3. 模拟实时音频输入与推理循环
    while (device_is_running) {
        // 从音频硬件设备获取 10ms ~ 20ms 的 PCM 数据 (如 RK3308 的 I2S 接口)
        std::vector<float> pcm_data = ReadAudioFromHardware();

        // 将音频喂入数据流
        stream->AcceptWaveform(16000, pcm_data.data(), pcm_data.size());

        // 轮询检查特征是否准备好，准备好则进行一次微小步长的解码
        while (spotter->IsReady(stream.get())) {
            spotter->Decode(stream.get());
        }

        // 获取解码结果
        auto result = spotter->GetResult(stream.get());

        // 判断是否成功检测到关键词
        if (!result.keyword.empty()) {
            printf("唤醒成功！检测到关键词: %s\n", result.keyword.c_str());

            // 触发后续操作：如亮灯、语音播报或启动大模型 ASR 识别
            TriggerWakeUpAction();

            // 重置状态机，准备下一次唤醒
            spotter->Reset(stream.get());
        }
    }
}
```

---

### 二、 核心类组成

在上述流程中，核心设计遵循了**逻辑与状态分离**的思想。主要有以下三大核心类：

1. **`KeywordSpotter` (唤醒词识别引擎类)**
    - **职责**：这是无状态的全局引擎。它负责持有加载在内存中的 ONNX 神经网络（如 Zipformer 权重）以及关键词的有限状态自动机（FSA Graph）。由于它是无状态的，因此可以被多个线程安全地共享，用于实现多路麦克风的并发识别。
2. **`OnlineStream` (在线音频数据流类)**
    - **职责**：这是有状态的上下文类。它维护了动态的环形缓冲区（Ring Buffer）、特征提取器（Feature Extractor）的当前状态、以及解码网络（RNN/LSTM 隐藏层）在特定时间步的中间计算状态。
3. **`FeatureExtractor` (特征提取类 - 通常作为 Stream 的成员)**
    - **职责**：负责将原始连续的 PCM 一维声波，经过预加重、分帧、加窗、FFT 等数字信号处理，转换为模型需要的二维 Fbank 声学特征张量（Tensor）。

---

### 三、 关键词识别执行过程 UML 序列图

此序列图展示了在实际执行关键词识别时，各个对象（类实例）之间的方法调用和控制流流转：

![[Pasted image 20260706003340.png]]

---

### 四、 线程模型设计与关系深度剖析

对于嵌入式边缘设备（如上述提到的 RK3308、树莓派等,），要保证 KWS 的极低延迟（Low Latency）且不漏音，**线程的解耦与流水线设计**是至关重要的。

在 `sherpa-onnx` 的实际工程部署中，通常会采用**经典的三线程/双线程模型**（这里用费曼技巧为您解释其角色分工）：

#### 1. 线程分类与职责

- **Thread A：音频采集线程 (Audio Capture Thread) —— “搬运工”**
    - **核心功能**：专门与硬件（如 ALSA 驱动、PortAudio）打交道。它是一个极其敏感的高优先级线程。只要麦克风有数据（例如每 10 毫秒生成一次数据），它就立刻调用 `AcceptWaveform` 将数据塞进 `OnlineStream` 的池子里,。
    - **禁忌**：这个线程绝对不能做任何复杂的数学计算或等待（如模型推理），否则会导致硬件缓冲溢出（Buffer Overrun），造成漏音。
- **Thread B：推理与解码线程 (Inference/Worker Thread) —— “质检员”**
    - **核心功能**：这是一个死循环或事件驱动的后台线程。它不断地调用 `IsReady()`，盯着 `OnlineStream` 的池子。一旦发现池子里的数据足够了，它就开始调用 `Decode()`，把数据送给 ONNX 神经网络进行矩阵运算。
    - **特点**：属于 CPU/NPU 密集型线程，主要消耗算力。
- **Thread C：业务主线程 (Application UI/Main Thread) —— “大堂经理”**
    - **核心功能**：负责最开始的资源初始化（`CreateKeywordSpotter`）。当 Thread B 的 `GetResult()` 发现了唤醒词，就会向这个主线程发送一个事件或信号。主线程收到后，负责执行相应的业务，比如调用系统 API 点亮屏幕、播放提示音等。

#### 2. 线程之间的关系与并发安全

- **Producer-Consumer（生产者-消费者）关系**：
    - **音频线程 (A)** 是生产者，**推理线程 (B)** 是消费者。
    - 它们之间的桥梁就是核心类 `OnlineStream`。`OnlineStream` 内部设计了线程安全的环形缓冲区（Ring Buffer）和互斥锁（Mutex）。
    - 音频线程可以异步地随时向 Stream 中塞入音频，而不会干扰正在调用 `Decode()` 进行矩阵运算的推理线程。
- **单模型与多线程并发复用 (Single Engine, Multiple Streams)**：
    - 如果有多个麦克风（如阵列麦克风），我们**不需要**在内存中创建多个庞大的 `KeywordSpotter` 神经网络模型。
    - 我们可以只启动**一个** `KeywordSpotter` 实例，但创建**多个** `OnlineStream` 实例。一个推理线程可以轮流对这多个 Stream 调用 `IsReady()` 和 `Decode()`，这极大程度地节约了嵌入式设备的内存开销（如 RAM 仅有 256MB/512MB 的 RK3308）。