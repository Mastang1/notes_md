
# 1. asr flow
![[Pasted image 20260630224538.png]]


# 2. sherpa-onnx
我们将训练好的大模型转换成一种轻量级、通用的标准格式，叫做 ONNX (Open Neural Network Exchange)。你可以把它理解为针对神经网络的 .elf 或 .bin 可执行文件。
	sherpa-onnx 就是一个纯 C/C++ 编写的、专门用来在各种设备上运行语音 ONNX 模型的引擎。 “Sherpa” 的意思是高山向导。语音 AI 的模型就像一座大山，而 sherpa-onnx 就是那个向导，帮你把笨重的 AI 模型轻松、高效地背到各种平台上（Android, iOS, 树莓派, ARM 开发板，甚至部分单片机），并且完全不需要依赖 Python。

>它的核心功能家族：

	ASR (语音识别)： 听懂你说什么。
	TTS (语音合成)： 朗读给你听。
	VAD (端点检测)： 像一个“声音开关”。判断当前有没有人在说话，没人说话时就挂起，节省 CPU 算力。
	KWS (关键词唤醒)： 比如“小爱同学”、“Hey Siri”，一直保持极低功耗监听，听到特定词才激活。

>其流程[[#1. asr flow]]

