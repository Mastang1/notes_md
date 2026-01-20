
# 1. manager process多次运行test case出现异常
![[Pasted image 20260115185214.png]]

这是一个非常典型且棘手的 Python `multiprocessing` 问题。

结合你的错误截图（`RecursionError`）和描述（运行约1000条 Case 后必现，且运行在 `BaseManager` 进程中），这**并不是**因为你的测试用例 `989` 内部真的写了一个无限递归的函数，而是一个**与 `multiprocessing.Manager` 通信机制相关的序列化（Pickle）深度问题**。

以下是深度分析和解决方案：

### 核心原因分析

**结论：** 这是一个由 **“代理对象嵌套（Nested Proxies）”** 或 **“累积引用（Accumulated References）”** 导致的 Pickle 序列化递归溢出。

#### 1. 现象解读

- **Case 989 并不是罪魁祸首：** 错误发生在第 1000 条左右，这是 Python 默认递归深度（通常为 1000）的临界点。这说明系统中有一个数据结构或调用链随着测试用例的执行在**不断变长/变深**，直到第 1000 次击穿了限制。
    
- **Traceback 的真相：**
    
    - 错误发生在 `func[4]()` 调用时。这是一个通过 `BaseManager` 获取的代理方法。
        
    - 调用栈进入了 `multiprocessing\managers.py` 的 `_callmethod`。
        
    - 最终在 `convert_to_error` 或数据传输阶段触发了 `RecursionError`。
        

#### 2. 技术机理 (为什么会炸？)

Python 的 `multiprocessing.Manager` 使用 `pickle` 模块在进程间传输数据。`pickle` 在处理链式结构或深层嵌套对象时，是递归执行的。

极有可能发生了以下两种情况之一：

- 情况 A：洋葱剥皮效应（代理套代理）
    
    你可能在每次测试结束后，没有彻底清理某些共享变量，而是将上一轮的某种“环境对象”或“上下文”传递给了下一轮。
    
    - 比如：`Context_2` 包含了 `Context_1` 的引用，`Context_3` 包含了 `Context_2` 的引用...
        
    - 到了第 1000 个 Case，`Context_1000` 实际上是一个深度为 1000 的链表。
        
    - 当 `getTcRunInfo` 试图返回这个 Context 或者访问它时，`pickle` 试图序列化这个深度为 1000 的对象，导致 `maximum recursion depth exceeded`。
        
- 情况 B：共享列表/字典的累积
    
    如果 BaseManager 中维护了一个全局的 List 或 Log 对象，并且每个 Case 都往里面塞入了一个复杂的对象（特别是包含 Proxy 的对象）。
    
    - 当 Case 989 运行时，如果代码无意中尝试读取、复制或打印这个包含 1000 个复杂对象的全局列表，Pickle 可能会因为遍历深度过大而崩溃。
        

---

### 排查与解决方案

建议按以下顺序进行排查：

#### 方案一：验证性修复（快速确认问题）

在你的 `tcf_test_runner.py` 或程序的入口处（主进程），手动增加 Python 的递归深度限制。

Python

```
import sys
# 默认通常是 1000，尝试设置得更大
sys.setrecursionlimit(5000) 
```

- **预期结果：** 如果设置后，程序能跑过 1000 条（比如在 5000 条时才崩），那么 **1000% 确认是对象嵌套过深的问题**。
    
- **注意：** 这只是治标不治本，长期运行还是会崩。
    

#### 方案二：检查代码中的“累积变量”（根治方案）

重点检查 `func[4]` (即 `getTcRunInfo`) 及其相关逻辑：

1. 检查入参和返回值：
    
    getTcRunInfo 是否返回了一个包含历史所有 Log 或历史 Context 的对象？
    
2. 检查 Setup/Teardown：
    
    在 BaseManager 所在的进程中，是否有一个全局变量（Global List/Dict）在不断 append 数据但从未 clear？
    
    - **特例检查：** 检查是否把 `Manager` 创建的 **Proxy 对象**（如 `ListProxy`, `DictProxy`）又存回到了另一个 **Manager 对象**中。这种“Proxy inside Proxy”非常容易导致序列化深度爆炸。
        

#### 方案三：重构进程管理策略（架构优化）

如果你的测试架构要求必须在 Manager 进程跑，且必须跑很久，建议采用 **“定期重启”** 策略：

- **设计：** 不要试图在一个 Manager 进程生命周期内跑完 10000 个 Case。
    
- 实现： 1. 每执行 100 个 Case（或者 500 个），主动销毁当前的 BaseManager。
    
    2. 重新启动一个新的 BaseManager 进程。
    
    3. 继续执行剩下的 Case。
    
    - 这不仅解决了递归深度问题，还能防止长时间运行导致的内存泄漏（Memory Leak）。
        

### 总结建议

你的代码并没有逻辑死循环。问题出在“状态管理”。

你在跨进程通信中，无意间构建了一个随 Case 数量线性增长的深层数据结构。

下一步行动：

请先尝试 方案一 (sys.setrecursionlimit(3000))。如果报错变成了 3000 条左右才出现，请立即着手清理 tcf_test_runner 中的全局累积变量，确保每个 Case 执行完后，不保留对上一个 Case 对象的引用。

# 2. 修改方案

[这里的图是指代码tcf_test_module.py中关于stdout等]
这是非常精彩的“破案”现场！结合你提供的三张截图，特别是 **图2 (Manager逻辑)** 和 **图3 (DualStdout类定义)**，根本原因已经完全确认。

这**不是**死循环逻辑错误，而是一个经典的 **“俄罗斯套娃” (Matryoshka Doll) 效应** 导致的引用链过深。

### 1. 根本原因深度剖析

#### 现象回顾

你的代码逻辑是：

1. **图2 第150行**：`sys.stdout = DualStdout()`。这一步把系统标准输出替换成了你的自定义类。
    
2. **图3 第16行**：在 `DualStdout` 初始化时，`self.console = sys.stdout`。它把“当前的标准输出”保存下来，以便把日志同时打印到控制台。
    
3. **图2 第163行**：在 `_post_redirect_std` 结束前，你再次调用了 `self._pre_redirect_std()`。
    

#### 致命链条 (The Chain of Death)

请看下述流程，它是如何杀死你的程序的：

- **第 1 个 Case 运行前：**
    
    - `sys.stdout` 是原始的 **Python原生控制台** (我们称之为 `Original_Stdout`)。
        
    - 调用 `_pre_redirect_std`。
        
    - 创建 `DualStdout_1`。它的 `self.console` 指向 `Original_Stdout`。
        
    - **现在的 `sys.stdout` 变成了 `DualStdout_1`**。
        
- **第 1 个 Case 结束 (调用 `getTcRunInfo`)：**
    
    - 执行到第163行，再次调用 `_pre_redirect_std`。
        
    - **关键点来了：** 代码执行 `sys.stdout = DualStdout()`。
        
    - 新创建 `DualStdout_2`。它的初始化函数执行 `self.console = sys.stdout`。
        
    - 此时系统的 `sys.stdout` 是谁？是 **`DualStdout_1`**！
        
    - 所以，`DualStdout_2` 里面包裹着 `DualStdout_1`。
        
    - **现在的 `sys.stdout` 变成了 `DualStdout_2`**。
        
- **第 989 个 Case：**
    
    - `sys.stdout` 是一个包裹了 988 层 `DualStdout` 的对象。
        
    - 链条：`DualStdout_989` -> `DualStdout_988` -> ... -> `DualStdout_1` -> `Original_Stdout`。
        

#### 为什么会报 RecursionError？

当你调用 **图2 第154行** `sys.stdout.flush()` 时：

1. `DualStdout_989.flush()` 被调用。
    
2. 它调用 `self.console.flush()` (见图3 第24行)。
    
3. 这会触发 `DualStdout_988.flush()`...
    
4. 这会一直向下递归调用，直到第 1000 层，Python 检测到递归栈溢出，程序崩溃。
    

---

### 2. 解决方案

你有两处修改方案，建议 **同时采纳** 以确保健壮性。

#### 方案一：修正重置逻辑（最直接的修复）

在 **图2** 的代码中，你注释掉了“还原现场”的代码（第159-162行）。你需要启用它们，或者在创建新对象前强制复位。

**修改文件：包含 `_post_redirect_std` 的文件**

Python

```
    def _post_redirect_std(self):
        sys.stdout.flush()
        sys.stderr.flush()
        ret_info = sys.stdout.getvalue()
        ret_info += sys.stderr.getvalue()
        
        # --- 修复开始 ---
        # 必须先还原回最原始的 stdout，打断“套娃”链条
        # 使用 sys.__stdout__ 可以获取 Python 启动时的原始控制台对象
        import sys
        sys.stdout = sys.__stdout__
        sys.stderr = sys.__stderr__
        # --- 修复结束 ---
        
        # 重新初始化，为下一个 case 做准备
        self._pre_redirect_std() 
        return ret_info
```

#### ==方案二：修正 DualStdout 类（更安全的防御）==

为了防止 `DualStdout` 意外包裹自己，建议修改 **图3** 中的 `__init__` 方法，强制它只包裹真正的物理控制台，而不是当前的 `sys.stdout`。

**修改文件：包含 `DualStdout` 类的文件**

Python

```
class DualStdout:
    def __init__(self):
        # 错误写法（会导致无限套娃）：
        # self.console = sys.stdout 
        
        # 正确写法（永远指向物理控制台）：
        self.console = sys.__stdout__ 
        self.stream = io.StringIO()

class DualStderr:
    def __init__(self):
        # 错误写法：
        # self.console = sys.stderr
        
        # 正确写法：
        self.console = sys.__stderr__
        self.stream = io.StringIO()
```

### 总结

这就是为什么程序在运行约 1000 次后必现崩溃的原因。你的 `DualStdout` 并不是在“复用”，而是在“无限叠加”。

推荐操作：

优先实施 方案二（修改 DualStdout 类），将 sys.stdout 改为 sys.____stdout____。这能从根本上杜绝该类发生递归嵌套的可能性，无论外层逻辑如何调用。
