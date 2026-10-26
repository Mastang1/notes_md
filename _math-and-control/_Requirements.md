# 1. python dependencies

作为 MATLAB 的轻量级替代方案，Python 的“SciPy 体系”提供了从底层矩阵运算、数值求解、符号推导到可视化的全套工具。

## 1. 环境部署 (Pip Commands)

在开始之前，建议一次性安装所有核心依赖库。请在您的终端或命令行（如 cmd, PowerShell, 或 Terminal）中运行以下命令：

```
# 核心科学计算四大件：矩阵计算、科学求解、绘图、符号计算
pip install numpy scipy matplotlib sympy

# 安装ipython用于交互运行一段脚本
pip install ipython
```

## 2. 核心替代库详解与 Demo

### 2.1 NumPy

- **核心功能**：高性能多维数组对象（ndarray）及矩阵运算。
    
- **用途说明**：对标 MATLAB 的基础矩阵操作。Python 原生列表不支持向量化计算，NumPy 是所有科学计算库的底层基石，用于生成时间轴、初始化状态矩阵以及批量数学运算。
    

**简单功能 Demo**：

```python
import numpy as np

# 生成从 0 到 2π 的 100 个等距节点 (类似 MATLAB 的 linspace)
x = np.linspace(0, 2*np.pi, 100)
# 向量化执行数学函数
y = np.sin(x)

print(f"数组维度: {y.shape}, 前三个元素: {y[:3]}")
```

### 2.2 SciPy

- **核心功能**：高级数值计算工具箱，包含积分、优化、插值、信号处理等。
    
- **用途说明**：对标 MATLAB 的各类 Toolboxes。在算法测试中，最常用的子模块是 `scipy.integrate`，用于求解常微分方程（ODE）的数值解（完美替代 `ode45`）。
    

**简单功能 Demo (求解一阶微分方程)**：

```python
import numpy as np
from scipy.integrate import solve_ivp

# 定义方程 dy/dt = -y + 1
def my_ode(t, y):
    return -y + 1

# 求解：时间 [0, 5]，初始值 y(0)=0
sol = solve_ivp(my_ode, t_span=[0, 5], y0=[0], t_eval=np.linspace(0, 5, 10))

print("时间点 (t):", sol.t)
print("系统状态 (y):", sol.y[0])
```

### 2.3 Matplotlib

- **核心功能**：2D/3D 数据与函数图像可视化。
    
- **用途说明**：对标 MATLAB 的 `plot` 系统。其 `pyplot` 接口的设计初衷就是为了方便 MATLAB 用户平滑过渡，API 非常相似。
    

**简单功能 Demo**：

```python
import numpy as np
import matplotlib.pyplot as plt

x = np.linspace(0, 10, 100)
y = np.exp(-0.2 * x) * np.cos(x)

plt.plot(x, y, 'r-', linewidth=2, label='Damped Oscillation')
plt.title('Matplotlib Test')
plt.xlabel('Time')
plt.ylabel('Amplitude')
plt.grid(True)
plt.legend()
# plt.show() # 在本地环境中运行以弹窗显示图像
```

### 2.4 SymPy

- **核心功能**：代数符号计算系统。
    
- **用途说明**：对标 MATLAB 的 Symbolic Math Toolbox。==🔴用于不需要带入具体数值的理论推导阶段==，如求导数、积分、化简公式，以及直接求微分方程的**解析解**（包含常数 C 的公式形式）。
    

**简单功能 Demo (求方程解析解)**：

```python
from sympy import symbols, Function, Eq, dsolve, diff

# 定义符号变量 t 和函数 f(t)
t = symbols('t')
f = Function('f')

# 建立微分方程： f'(t) - f(t) = 0
ode = Eq(diff(f(t), t) - f(t), 0)

# 求解解析解
solution = dsolve(ode)
print("解析解为:", solution) 
# 输出结果形如: Eq(f(t), C1*exp(t))
```