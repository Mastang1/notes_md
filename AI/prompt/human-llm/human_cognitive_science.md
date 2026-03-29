下面这套东西，不是某一个单一学派的“标准答案”，而是把认知科学、学习科学、教育心理学和教学设计中的高价值结论，压缩成一个更适合做 **LLM 教学系统** 的工程化综合框架。核心共识很稳定：人类理解陌生事物，不是靠一次性接收定义，而是靠 **把新信息挂到已有知识上、形成概念框架、控制工作记忆负荷、修正错误直觉、通过例子与检索练习稳固并迁移**。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))
---
### 前置概念，表征、图式、心智模型
- 表征：👉 “它长什么样”
- 图式：👉 “它一般怎么组织”
	-  *为什么专家比新手快？（图式多）*
	- *为什么会刻板印象？（图式过度泛化）*
- 心智模型：👉 “它为什么这样运作”
---
Video lesson![[如何真正理解任何事.mp4]]
PPT![[Engineering_Cognitive_Scaffolds.pdf]]


## 1. 宏观学科地图：哪些科研领域在研究“人类如何理解新事物”

### 1.1 三层学科结构

可以把相关研究分成三层：

1. **认知结构层**：人脑怎样表征、编码、提取、组织新知识。
    
2. **学习机制层**：什么条件下，新知识更容易被理解、修正、巩固、迁移。
    
3. **教学设计层**：怎样把这些规律变成可执行的教学顺序、任务设计和反馈机制。 ([National Academies](https://www.nationalacademies.org/read/24783/chapter/2 "Read \"How People Learn II: Learners, Contexts, and Cultures\" at NAP.edu"))
    

### 1.2 领域地图表

|层级|领域|核心研究对象|与“理解陌生事物”的关系|对 LLM 生成教程的启发|代表依据|
|---|---|---|---|---|---|
|认知结构层|Cognitive Science（认知科学）|注意、记忆、表征、推理、问题求解|解释人为什么会“看懂字但没形成理解”|先帮用户建表征，再谈深层原理|([National Academies](https://www.nationalacademies.org/read/24783/chapter/2 "Read \"How People Learn II: Learners, Contexts, and Cultures\" at NAP.edu"))|
|认知结构层|Human Memory Models（记忆模型）|工作记忆、长时记忆、提取|新知识必须先穿过有限工作记忆，才能进入稳定结构|限制每轮新术语密度，分段输出|([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))|
|认知结构层|Schema Theory（图式理论）|先验知识如何组织并吸收新信息|陌生事物的理解，本质上是“旧图式吸收/改造新信息”|讲解前先找用户熟悉锚点|([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))|
|认知结构层|Mental Models（心智模型）|人如何形成对系统/机制的内部因果图|用户真正理解时，脑中会出现“这玩意怎么运转”的模型|教程不能只给定义，要给因果链和运行图景|([Jacksonville State University](https://www.jsu.edu/online/faculty/MULTIMEDIA%20LEARNING%20by%20Richard%20E.%20Mayer.pdf "PII: S0079-7421(02)80005-6"))|
|认知结构层|Knowledge Representation（知识表征）|知识是如何被编码成概念、关系、层级、图|决定“用户记住的是孤立点，还是结构网络”|教程要显式输出“概念—关系—边界—例子”|([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))|
|学习机制层|Learning Science（学习科学）|有效学习策略及其证据|直接回答“什么方法更能让陌生知识被真正学会”|采用检索、间隔、例题、自解释等高证据策略||
|学习机制层|Educational Psychology（教育心理学）|学习动机、反馈、理解、误解、迁移|连接认知规律与课堂/教学情境|教程要兼顾理解、动机、反馈和自我监控|([ERIC](https://eric.ed.gov/?id=EJ732415 "ERIC - EJ732415 - Problem-Based Learning: What and How Do Students Learn?, Educational Psychology Review, 2004-Sep"))|
|学习机制层|Cognitive Load Theory（认知负荷理论）|有限工作记忆下的教学设计|陌生对象最怕“元素交互过高”导致认知过载|先分块，再串联；先低负荷表征，再上复杂机制|([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))|
|学习机制层|Constructivism（建构主义）|学习者主动建构意义|新知识不是被灌入，而是被主动组织|LLM 不应只讲，应不断要求用户连接旧知、做解释|([ERIC](https://eric.ed.gov/?id=EJ732415 "ERIC - EJ732415 - Problem-Based Learning: What and How Do Students Learn?, Educational Psychology Review, 2004-Sep"))|
|学习机制层|Conceptual Change Theory（概念转变）|错误直觉如何被修正|对陌生事物，用户常会先套错旧概念|要显式讲“它不是什么”“常见误解为何错”|([Eurasia Journal](https://www.ejmste.com/download/an-overview-of-conceptual-changetheories-4082.pdf "Microsoft Word - EJMSTE_v3n4_Ozdemir_Clark"))|
|学习机制层|Metacognition（元认知）|学习者是否知道自己懂到哪一步|很多人“以为懂了”，其实只是熟悉表面词汇|LLM 要插入理解检测、自评、反思问题|([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC6755210/ "Knowledge of Learning Makes a Difference: A Comparison of Metacognition in Introductory and Senior-Level Biology Students - PMC"))|
|学习机制层|Transfer of Learning（迁移）|是否能把学到的东西用到新情境|真理解的标志不是复述，而是换场景还能用|教程末尾必须有类比迁移、变式题、边界案例|([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))|
|学习机制层|Expertise Development（专家发展）|新手如何走向熟练与专家|新手和熟手需要的帮助不同|LLM 必须按“新手模式”讲，并随理解升级逐步撤支架|([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S0959475225000660 "A cornerstone of adaptivity – A meta-analysis of the expertise reversal effect - ScienceDirect"))|
|学习机制层|Analogical Learning（类比学习）|通过熟悉结构理解陌生结构|陌生对象常先靠类比建立第一层表征|先用类比开门，但随后必须指出类比失效边界|([ScienceDirect](https://www.sciencedirect.com/science/article/abs/pii/S0361476X09000381 "Learning by analogy: Discriminating between potential analogs - ScienceDirect"))|
|教学设计层|Instructional Design（教学设计）|内容顺序、任务设计、反馈设计|把理论落成教程骨架|LLM 要有固定生成顺序，而不是想到哪讲到哪|([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))|
|教学设计层|Scaffolding（脚手架教学）|临时支持如何帮助完成超当前能力的任务|陌生知识的早期理解需要外部支撑|先高支撑：提示、分步、半成品；后期逐步撤掉|([Springer](https://link.springer.com/chapter/10.1007/978-3-319-02565-0_2 "Instructional Scaffolding: Foundations and Evolving Definition \| Springer Nature Link"))|
|教学设计层|Zone of Proximal Development（最近发展区）|什么是“自己不会，但在帮助下能会”的区间|最有效教学不在太简单，也不在完全听不懂|LLM 要把难度压在“有点难，但能跟上”|([Springer](https://link.springer.com/article/10.1186/s40594-024-00490-7?utm_source=chatgpt.com "Employing technology-enhanced feedback and scaffolding to ..."))|
|教学设计层|Worked Examples（例题/范例学习）|新手从完整示范中学会解决方式|对新手，先看完整范例通常优于先硬做题|教程先给最小完整例子，再让用户改/做|([ScienceDirect](https://www.sciencedirect.com/science/article/abs/pii/S0361476X1000055X "Effects of worked examples, example-problem, and problem-example pairs on novices’ learning - ScienceDirect"))|
|教学设计层|Retrieval Practice / Spaced Repetition / Desirable Difficulties|提取、间隔、适度困难|理解要靠反复提取和带一点困难的回忆稳固|教程要插入回忆题、延迟复盘、渐增难度||
|教学设计层|Problem-Based Learning（问题驱动学习）|在问题求解中学习概念与策略|当基础稍有建立后，问题会推动深层理解|先低门槛理解，再用问题把知识“逼活”|([ERIC](https://eric.ed.gov/?id=EJ732415 "ERIC - EJ732415 - Problem-Based Learning: What and How Do Students Learn?, Educational Psychology Review, 2004-Sep"))|
|教学设计层|Situated / Embodied Cognition（情境/具身认知）|理解与具体任务、情境、工具使用相连|脱离情境的解释容易变空|教程应绑定真实使用场景、任务流、实例|([Springer](https://link.springer.com/article/10.1007/BF02319856?utm_source=chatgpt.com "An instructional design framework for authentic learning ..."))|

### 1.3 哪些理论最适合直接指导 LLM 教学系统

如果只挑最值得吸收的，不是“所有理论平均用力”，而是这几组最实用：

- **图式/先验知识 + 概念框架 + 心智模型**：决定用户能不能把新东西装进脑子里。
    
- **认知负荷 + worked examples + 脚手架**：决定教程会不会把人讲懵。
    
- **概念转变 + 对比 + 反驳误解**：决定用户会不会把新东西理解偏。
    
- **检索练习 + 间隔 + 自解释 + 迁移任务**：决定理解能不能稳固并可用。
    
- **专家发展 / expertise reversal**：决定教程是否会随着用户水平变化而变。 ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608025001207 "How does prior knowledge affect learning? A review of 16 mechanisms and a framework for future research - ScienceDirect"))
    

### 1.4 几组容易混淆的理论比较

**建构主义**强调“理解是主动建出来的”；**认知负荷理论**强调“别把工作记忆压爆”。前者告诉你“学习不是灌输”，后者告诉你“但也不能放养”。对 LLM 来说，正确做法不是极端自由探索，也不是极端灌定义，而是 **先提供结构化支架，再逐步把主动建构交还给用户**。([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC10730747/?utm_source=chatgpt.com "3How do constructivism learning environments generate ..."))

**图式理论**关注“新知识怎样接到旧知识上”；**概念转变理论**关注“旧知识如果是错的，怎样改掉”。前者解决“怎么吸收”，后者解决“怎么纠偏”。所以 LLM 不只要做连接，还要做边界与纠错。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

**脚手架/ZPD**和 **worked examples** 也不同。脚手架是“支持方式”的总框架，worked examples 是其中对新手特别有效的一类具体手段。([Springer](https://link.springer.com/chapter/10.1007/978-3-319-02565-0_2 "Instructional Scaffolding: Foundations and Evolving Definition | Springer Nature Link"))

---

## 2. 人类认识“新事物”的认知过程模型

下面这个模型不是某篇论文原封不动的阶段表，而是对先验知识、心智模型、概念转变、例题学习、迁移研究做的综合工程化归纳。它更适合拿去设计 LLM 教学流程。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

### 2.1 通用认知阶段模型

|阶段|目标|学习者心理状态|常见困难|最适合的教学方式|LLM 该怎么讲|
|---|---|---|---|---|---|
|0. 定向与动机|先知道“为什么要学它”|茫然、无抓手|不知价值，注意力不稳|用用途、问题、场景开场|先说“它解决什么痛点”，别先下定义|
|1. 命名与边界|知道“它是什么/不是什么”|开始分类|容易混同相近概念|对比定义、反例、非例|用“它像X，但不是Y”|
|2. 粗粒度整体印象|形成整体地图|仍是模糊轮廓|细节太多导致失焦|先整体后局部，概念地图|先给全景图，再告诉后面会展开什么|
|3. 建立第一版心智模型|形成初步因果或结构理解|“似懂非懂”|只能背词，不能解释运作|类比、简化机制图、最小系统|用熟悉事物做类比，但立即指出限制|
|4. 连接旧知并重组|把新知识接入已有图式|开始能挂钩|套用错误旧经验|前置知识激活、显式映射|说“如果你会A，可以把这里理解成B中的C”|
|5. 拆分关键组成部分|学会部件、角色、关系|有一定抓手|知识碎片化|分块、标签化、局部讲解|一次只讲少量核心术语|
|6. 理解动态机制|从静态部件走向运行过程|开始能追流程|因果链断裂、机制难跟|流程化解释、因果链、时序示例|用“先…再…因此…”讲运行逻辑|
|7. 实例化与辨析|会在例子中认出它|理解开始落地|只会抽象说法，不会识别实例|worked example、对比例、边界案例|给最小例子，再给相似但不同的例子|
|8. 操作与反馈|从“知道”走到“会用”|有试错欲望|一用就错，暴露理解漏洞|guided practice、反馈、自解释|先让用户修改例子，再让其独立完成|
|9. 迁移与内化|能解释、应用、比较、迁移|进入稳固期|只能在原题型里用|变式任务、跨情境迁移、检索|让用户解释给别人听，或应用到新场景|

这套阶段模型背后的核心逻辑是：**陌生知识的理解不是线性“听完=学会”，而是从命名、分类、表征、机制、实例、操作、迁移逐层推进**。如果缺了前面任何一层，后面往往会出现“会复述但不会用”“会用模板但不会迁移”“能做题但讲不明白原理”的假理解。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

### 2.2 通用流程图

**问题/用途定向**  
→ **命名与边界**  
→ **整体地图**  
→ **类比与初始心智模型**  
→ **关键组成部分**  
→ **动态机制/因果链**  
→ **最小例子**  
→ **对比与反例**  
→ **引导练习与反馈**  
→ **检索复述**  
→ **迁移应用**。 ([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

---

## 3. 影响“新知识理解效果”的关键科学原理

下面只挑 **高价值、可落地、适合工程实现** 的原理。很多口号式说法都能在这里找到证据边界：有些是强证据结论，有些只是常用启发式，不该被当成铁律。([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))

|原理|为什么有效|解决什么问题|LLM 教学落地策略|
|---|---|---|---|
|激活前置知识|新知识靠旧图式吸收；不激活就难挂接|“听懂字面但无内在连接”|先问：你熟悉哪个相邻概念？再映射讲解|
|先给概念框架，再灌细节|框架化组织能提升提取与迁移|碎片化、记不住、不会迁移|先给全景图/层次图，再逐层展开|
|控制认知负荷|工作记忆有限，陌生知识易超载|一上来就懵、术语堆积|限制每轮新术语；长解释拆成 3–5 块|
|分块（chunking）|把多个元素压成更高层单元|元素交互太多，跟不上|按“概念块/流程块/部件块”讲，不连续抛点|
|渐进展开（progressive disclosure）|先低分辨率，再提高分辨率|细节淹没整体|第一轮只给必要信息，后续按需加层|
|worked examples 优先于先裸做题（对新手）|新手先看完整范例，通常比先硬解更有效|会被问题求解卡死|先给一个最小完整例子，再让用户改例子|
|自解释（self-explanation）|迫使学习者把信息组织成自己的因果解释|以为懂了，实际不会解释|每讲完一段，要求用户用自己的话解释|
|对比学习与边界辨析|相似概念并排更能看出深层差异|混淆相近概念|必给“它 vs 相近对象”的差异表|
|错误概念显式修正|误解不会自动消失，需要反驳与重建|旧直觉套错|用“常见误解—为什么错—正确说法”|
|多重表征|文本、图、流程、类比、案例从不同通道支持理解|单一表达吃不透|同一概念至少给：定义 + 例子 + 机制 + 对比|
|检索练习|回忆本身能强化学习，不只是测验|看懂当场，过后忘光|插入“闭卷回忆”问题，而不是只让继续阅读|
|间隔复习|分散重提比一次性重复更稳固|短期懂、长期丢|在教程尾部附“1小时/1天/3天复盘问题”|
|适度困难|稍费力的提取和应用更利于长期保持|过于顺滑导致假熟悉|让用户先猜、先判别、先做小迁移|
|问题驱动学习|当基础表征已有后，问题可推动深层理解|只会记结论，不会用|基础讲完后给一个真实问题来套用|
|脚手架 + 逐步撤除|早期多支撑，后期减少支撑更利于独立|一直依赖提示，不能独立|例子→半填空→独立回答|
|按用户水平自适应|新手/熟手最优教学不同|新手被抽象压死，熟手被冗余拖慢|默认新手模式，检测到会了再升级密度|

这些原理的证据强度并不完全一样。**认知负荷、worked examples、检索练习、间隔学习、前置知识、概念框架、expertise reversal** 的证据最硬；**“先用途后原理”“先整体后局部”** 更像高频有效的教学启发式，但并非在所有材料上都自动优于别的顺序。一个 2024 研究甚至发现，成人虽然更偏好“功能先于机制、整体先于部分”的解释顺序，但这种偏好本身并不稳定等同于更好的学习效果。也就是说，**顺序要服务于表征建立，不是迷信某个固定句式**。([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))

---

## 4. 针对“完全无基础用户”的最佳教学组织方式

### 4.1 通用教程骨架

下面这套骨架，适合 Redis、Transformer、Docker、卡尔曼滤波、AUTOSAR、协程、贝叶斯更新这类“用户可能完全陌生”的对象。它不是按学科分类定制，而是按 **陌生对象被人类理解的顺序** 组织。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

|顺序|教程部分|为什么放这里|对应原理|
|---|---|---|---|
|1|这东西是干什么的|先建立动机和用途坐标|定向、前置知识激活|
|2|它解决什么问题|用问题定义对象，比空定义更抓人|问题驱动、情境认知|
|3|为什么会出现它|建立历史/需求背景，避免把它看成孤立词条|概念框架、情境化理解|
|4|它不是什么 / 与谁不同|先立边界，减少误归类|概念转变、对比学习|
|5|一个熟悉类比|给新手第一块认知踏板|类比学习、图式连接|
|6|核心整体图景|在细节前先给地图|先框架后细节、负荷控制|
|7|核心术语最小集|只引入理解后面内容所必需的词|术语控量、渐进展开|
|8|核心组成部分|从整体到主要部件|分块、结构化表征|
|9|它如何工作|把部件放进因果链/流程中|心智模型、动态机制|
|10|一个最小可运行例子|抽象必须落地成实例|worked example、实例化|
|11|常见误解与反例|防止早期形成错误图式|概念转变、反驳文本|
|12|使用边界/何时别用|真理解一定包含适用范围|辨析、迁移边界|
|13|与已有知识体系的关系|帮用户把它挂入更大知识网|概念框架、迁移|
|14|练习：复述 / 对比 / 应用|把“看懂”转成“可提取、可操作”|检索、自解释、练习|
|15|进阶路径|从新手迈向熟练的路线图|专家发展、脚手架撤除|
|16|如何判断自己真的理解了|用可操作标准收尾|元认知、迁移检验|

### 4.2 为什么这是更好的顺序

这个顺序背后的硬逻辑是：

- **先用途/问题/边界**，让用户先找到对象在世界中的位置。
    
- **再类比/整体图景/最小术语集**，先形成可运行的低分辨率模型。
    
- **再部件与机制**，把对象从“名字”升级为“系统”。
    
- **再例子、误解、边界**，让理解不漂浮、不跑偏。
    
- **最后用检索、比较、迁移收口**，把理解从被动识别推进到主动调用。 ([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))
    

---

## 5. 将科研理论转化为 LLM 教学策略

下面是最关键的部分：把理论压成 **LLM 可执行的讲解算法**。这是工程化版本，不再停留在学术名词。该框架主要是我基于上述研究做的综合设计。([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))

### 5.1 LLM 教学策略框架

#### 策略层 1：先判断对象类型

把陌生对象分成四类：

1. **工具/系统类**：如 Docker、Redis、AUTOSAR。
    
2. **机制/算法类**：如 Transformer、卡尔曼滤波、贝叶斯更新。
    
3. **概念/范式类**：如协程、幂等性、依赖注入。
    
4. **库/框架类**：如某个开源库。
    

不同类型决定开场方式不同：  
工具/系统类优先 **用途—问题—场景**；  
机制/算法类优先 **现象/问题—直觉—简化机制**；  
概念类优先 **边界—对比—典型例子**；  
库/框架类优先 **它提供什么能力—和原生方案相比多了什么**。 ([ERIC](https://eric.ed.gov/?id=EJ732415 "ERIC - EJ732415 - Problem-Based Learning: What and How Do Students Learn?, Educational Psychology Review, 2004-Sep"))

#### 策略层 2：先判断用户所处阶段

至少分四档：

- **0级：完全无基础**：不知道名字、用途、边界。
    
- **1级：听说过**：知道大概用途，但没有机制模型。
    
- **2级：能复述**：会说术语，但不会应用或比较。
    
- **3级：能用但不会迁移**：会照着做，但不能解释何时适用。
    

默认把陌生对象教学从 **0级新手模式** 开始，因为对新手“多支撑”通常比“少支撑”更重要；这正符合 expertise reversal 的方向。([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S0959475225000660 "A cornerstone of adaptivity – A meta-analysis of the expertise reversal effect - ScienceDirect"))

#### 策略层 3：先建立最小可运行表征

LLM 的第一任务，不是“完整讲对”，而是“先帮用户形成第一版不会崩的表征”。做法是：

- 用一句用途定义定位对象；
    
- 给一个熟悉类比；
    
- 给一个边界对比；
    
- 给一个最小例子。
    

只要这四件事没做，直接讲内部机制，通常就是越讲越糊。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

### 5.2 讲解顺序决策逻辑

#### 规则 1：类比优先还是定义优先

- 若对象 **高度陌生、抽象、不可见**，先类比再定义。
    
- 若对象 **用户已有强相邻知识**，可先给简定义，再立刻映射到旧知。
    
- 若对象 **定义本身就依赖更多术语**，绝不要定义优先。 ([ScienceDirect](https://www.sciencedirect.com/science/article/abs/pii/S0361476X09000381 "Learning by analogy: Discriminating between potential analogs - ScienceDirect"))
    

#### 规则 2：整体图景优先还是具体案例优先

- 若内容是 **系统/框架/架构**，先整体图景。
    
- 若内容是 **API/算法/操作技能**，先最小例子。
    
- 若内容元素交互很高，先整体再局部；若动作性很强，先示例再抽象。 ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))
    

#### 规则 3：术语密度怎么控

- 每个新段落只引入 **少量核心新术语**；
    
- 新术语一出现，立刻给角色说明；
    
- 在用户未形成表征前，不要连发术语定义链。 ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))
    

#### 规则 4：如何避免一上来过度抽象

先回答这四问，再谈抽象原理：  
**它干什么？解决什么问题？像什么？最小例子是什么？**  
四问没答，抽象定义大概率无处着陆。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

#### 规则 5：如何识别用户尚未建立基础表征

出现以下信号，就说明还不能上深层机制：

- 用户只能复述术语，不能举例；
    
- 能举例，但分不清边界；
    
- 会说流程，但说不出“为什么”；
    
- 一换场景就不会。
    

LLM 一旦检测到这些信号，应退回上一层：**从机制退回例子，从例子退回边界，从边界退回用途与类比**。([PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC6755210/ "Knowledge of Learning Makes a Difference: A Comparison of Metacognition in Introductory and Senior-Level Biology Students - PMC"))

### 5.3 用户理解逐步递进的讲解机制

可以把 LLM 讲解机制设计成四级爬梯：

**Level 1：识别级**  
用户能回答：这是什么、做什么、和谁不同。

**Level 2：模型级**  
用户能解释：它由什么组成、怎么运行、为什么这样设计。

**Level 3：操作级**  
用户能完成：最小例子、变式任务、错误修正。

**Level 4：迁移级**  
用户能比较：什么时候用、什么时候不用、和相邻方案如何取舍。

只有当用户通过当前层检验，才进入下一层。([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

### 5.4 可直接写进 prompt / agent policy 的教程生成规则

**生成规则 v1：**

1. 先判定对象类型。
    
2. 先假设用户是新手。
    
3. 先输出用途、问题、边界、类比、最小例子。
    
4. 再输出整体框架。
    
5. 再讲核心部件与动态机制。
    
6. 每一层都要控制新术语数量。
    
7. 必须加入相近概念对比。
    
8. 必须加入常见误解与反例。
    
9. 必须加入一个 worked example。
    
10. 必须加入至少一次自解释或检索提问。
    
11. 必须加入迁移判断题：何时用、何时不用。
    
12. 若用户答错，优先回退并重建表征，而不是继续往下讲。 ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))
    

---

## 6. 最终结论：**面向 LLM 的陌生事物教学设计知识框架**

### 6.1 最值得掌握的核心学科与理论

最该吸收的不是“全部教育理论”，而是这五组：

1. **先验知识 / 图式 / 概念框架**
    
2. **认知负荷 / worked examples / 脚手架**
    
3. **概念转变 / 对比 / 反驳误解**
    
4. **检索练习 / 间隔 / 自解释 / 迁移**
    
5. **专家发展 / 自适应教学**。 ([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))
    

### 6.2 最关键的认知阶段模型

不是“定义 → 细节 → 练习”，而是：

**用途定向 → 边界分类 → 整体框架 → 类比建模 → 部件理解 → 动态机制 → 最小例子 → 对比与误解修正 → 引导练习 → 检索复述 → 迁移应用。** ([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

### 6.3 最重要的教学设计原则

- 先挂接旧知，再给新知。
    
- 先建表征，再上机制。
    
- 先控制负荷，再追求完整。
    
- 新手先看范例，不先扔裸题。
    
- 必讲边界、误解、反例。
    
- 真理解必须以检索、解释、迁移来验证。 ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608025001207 "How does prior knowledge affect learning? A review of 16 mechanisms and a framework for future research - ScienceDirect"))
    

### 6.4 最适合 LLM 的教程生成流程

**识别对象类型**  
→ **估计用户先验知识**  
→ **用途/问题/边界/类比/例子起步**  
→ **整体框架**  
→ **部件与机制**  
→ **对比与误解修正**  
→ **guided practice**  
→ **检索与自解释**  
→ **迁移与进阶路径**。 ([Springer](https://link.springer.com/chapter/10.1007/978-3-319-02565-0_2 "Instructional Scaffolding: Foundations and Evolving Definition | Springer Nature Link"))

### 6.5 后续值得深入研究的方向/关键词

继续深挖，优先看这些关键词：

- prior knowledge / schema activation
    
- cognitive load theory / element interactivity
    
- worked example effect / example-problem pairs
    
- conceptual change / refutation text
    
- self-explanation / generative learning
    
- retrieval practice / spacing / pretesting
    
- scaffolding / ZPD / fading
    
- expertise reversal effect
    
- mental models / causal models
    
- transfer of learning / far transfer。 ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608025001207 "How does prior knowledge affect learning? A review of 16 mechanisms and a framework for future research - ScienceDirect"))
    

---

## A. “理论 -> LLM 教程策略”映射表

|理论/原则|核心思想|对陌生知识教学的意义|对 LLM 的具体要求|可转化成的 Prompt 规则|
|---|---|---|---|---|
|图式理论|新知要接到旧知上|不先找锚点就难理解|先询问或假设相邻已知物|“先用用户熟悉的概念作映射”|
|概念框架|知识要有结构网络|防止碎片化|先给整体地图|“先输出全景，再展开细节”|
|认知负荷理论|工作记忆有限|防止术语轰炸|控制单轮信息量|“每轮不超过少量新术语”|
|worked examples|新手先学范例更有效|降低入门门槛|先给最小完整示例|“先举一个完整例子，再让用户改例子”|
|脚手架/ZPD|先帮，再撤帮|维持合适难度|分步引导，逐渐减少提示|“先给提示，再半提示，再独立任务”|
|概念转变|误解要显式修正|防止错图式固化|加入误解与反例|“列出常见误解，并说明为何错”|
|类比学习|用熟悉结构理解陌生结构|快速建立初始表征|先类比，后补边界|“先用类比，再指出类比哪里失效”|
|自解释|学习者解释时更深加工|防止假理解|插入复述/解释任务|“每个模块后要求用户用自己的话解释”|
|检索练习|回忆本身强化记忆|稳固理解|不只讲，要测提取|“每讲完一段给闭卷回忆题”|
|迁移学习|真理解能换场景使用|区分记住和掌握|给变式与边界任务|“给一个新情境，要求用户判断是否适用”|

---

## B. “错误讲解方式”的反例清单

|错误方式|为什么不利于理解|
|---|---|
|一开始就定义轰炸|用户还没有表征和锚点，定义只能变成噪音|
|没有整体图景就进细节|细节无挂点，记忆和迁移都差|
|不区分前置知识|新手被压死，熟手被拖慢|
|只讲“是什么”，不讲“解决什么问题”|用户找不到意义坐标，动机和定位都弱|
|只讲概念，不讲边界|最容易把相似对象混为一谈|
|只讲静态结构，不讲动态过程|用户不会形成真正的心智模型|
|没有最小例子|抽象概念无法落地|
|没有反例和误解修正|错误直觉会借旧图式固化|
|没有理解检测|用户和系统都可能误判“已经学会”|
|一直高强度提示，不逐步撤支架|用户只能跟着走，无法独立使用|
|只让用户重复阅读，不让其回忆|熟悉感会伪装成理解|
|讲得特别顺滑、完全无难度|短期舒服，长期记不住、不会迁移|

这些反模式几乎都能在上面的理论中找到解释：它们要么没有处理前置知识和表征，要么压爆了认知负荷，要么没有驱动学习者做主动加工和检索，要么没有设计边界与迁移。([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608024000165 "Cognitive load theory and individual differences - ScienceDirect"))

---

## C. 最终可执行结论：做“LLM 自动生成陌生知识教程”系统，最该吸收的 10 条原则

按重要度排序：

1. **先估计用户先验知识，再决定讲法。**
    
2. **先给用途、问题、边界，不要先扔定义。**
    
3. **先建立最小可运行表征，再上深层机制。**
    
4. **先整体框架，后局部细节；但操作型内容要尽早给最小例子。**
    
5. **严格控制术语密度和单轮认知负荷。**
    
6. **对新手优先使用 worked example 和脚手架。**
    
7. **必须显式讲“它不是什么”“常见误解为何错”。**
    
8. **每一层讲解后都要加入自解释或检索提问。**
    
9. **必须设置对比、反例、变式，检验是否真的理解。**
    
10. **讲解要能随用户水平升级：从高支撑到低支撑。** ([ScienceDirect](https://www.sciencedirect.com/science/article/pii/S1041608025001207 "How does prior knowledge affect learning? A review of 16 mechanisms and a framework for future research - ScienceDirect"))
    

一句话收束：

**面向陌生事物的优质 LLM 教学，不是“把知识说全”，而是“按人类建立表征、形成框架、修正误解、完成迁移的顺序，把知识说进脑子里”。** ([National Academies](https://www.nationalacademies.org/read/9457/chapter/4 "Read \"How People Learn: Bridging Research and Practice\" at NAP.edu"))

下一步最有价值的，不是继续空谈理论，而是把这套框架直接压成一份 **可投喂 NotebookLM / ChatGPT Thinking / Agent system 的系统 Prompt 模板**。