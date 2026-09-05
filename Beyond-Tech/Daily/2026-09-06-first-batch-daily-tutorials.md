# First-Batch Daily Tutorial System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first production-quality batch of the 20-week daily tutorial system: course guide, source index, reusable daily template, Week 1 seven detailed tutorials, and the DeviceTree deep-dive.

**Architecture:** The tutorial system is split into Daily Tutorial documents and Deep Dive documents. Daily tutorials answer “what to learn and do today”; Deep Dives hold reusable mechanism explanations so knowledge is not duplicated. All hardware-specific statements must be grounded in the user’s STM32F407 Explorer schematic or i.MX6ULL source material, while kernel/Zephyr mechanisms must be grounded in official documentation/source paths.

**Tech Stack:** Markdown, Mermaid, UML sequence diagrams, Linux/Zephyr shell commands, i.MX6ULL BSP workflow, STM32F407ZET6/Zephyr workflow, Linux DeviceTree/Driver Model.

**Spec:** `/mnt/data/docs/superpowers/specs/2026-09-06-daily-tutorial-system-design.md`

## Global Constraints

- First delivery scope is only the course guide/template, Week 1 seven tutorials, and `A01_DeviceTree_从DTS到LinuxDevice.md`.
- Linux track uses the 正点原子 i.MX6ULL platform.
- Zephyr track uses the uploaded 正点原子 Explorer STM32F4 board schematic; MCU is `STM32F407ZET6`.
- STM32F4 hardware facts must come from `Explorer STM32F4_V2.2_SCH.pdf`, not assumptions from similar boards.
- Daily tutorials must be instructional content, not outlines.
- Each Daily Tutorial must include: positioning, engineering problem, theory, experiment, fault injection, debugging path, explicit Pass/Fail acceptance, and Git deliverables.
- Difficult concepts use a two-layer explanation: 30-second Feynman model followed by precise engineering model.
- Mermaid diagrams must explain capability placement or structure; UML sequence diagrams are used only when ordering and participants matter.
- Local PDF references must include source ID plus chapter/page where available; external mechanisms prefer vendor/kernel/Zephyr official documentation.
- Each main daily learning block must be executable in about 2 hours; nonessential material must be marked optional/extension.
- No AI Runtime, AUTOSAR, Ethernet OTA, custom bootloader, or unrelated kernel source-code expansion in this first batch.
- DeviceTree deep-dive must explicitly explain `DTS → DTB → struct device_node → bus-specific device object → struct device → match/probe`, and Linux vs Zephyr Devicetree differences.

---

# File Structure

The implementation creates the following files:

```text
/mnt/data/soc-system-course/
│
├── README.md
├── 00_course_guide/
│   ├── learning_method.md
│   ├── environment_baseline.md
│   ├── hardware_inventory.md
│   ├── source_index.md
│   └── glossary.md
├── 01_templates/
│   └── daily_tutorial_template.md
├── 02_linux/week01/
│   ├── W01D01_Ubuntu主开发环境.md
│   ├── W01D02_交叉编译与ELF基线.md
│   ├── W01D03_6ULL串口与网络链路.md
│   ├── W01D04_TFTP与NFS开发闭环.md
│   └── W01D07_Week1复盘与环境复现.md
├── 03_zephyr/week01/
│   ├── W01D05_Zephyr官方环境.md
│   └── W01D06_F407硬件审计与BoardPort准备.md
├── 04_deep_dive/
│   └── A01_DeviceTree_从DTS到LinuxDevice.md
└── references/
    └── README.md
```

Responsibilities:

- `README.md`: course navigation and first-batch index only.
- `learning_method.md`: how to execute a day, review, log, and self-test.
- `environment_baseline.md`: canonical host/target environment and version-recording rules.
- `hardware_inventory.md`: only verified board resources and their learning use.
- `source_index.md`: stable source IDs and local/online reference mapping.
- `glossary.md`: terms reused across Week 1 and DeviceTree without duplicating explanations.
- `daily_tutorial_template.md`: required section schema and diagram/reference conventions.
- Week 1 files: complete executable daily tutorials.
- `A01_DeviceTree_从DTS到LinuxDevice.md`: reusable mechanism deep-dive.
- `references/README.md`: expected local reference filenames and relative-link convention.

---

### Task 1: Build course root and navigation

**Files:**
- Create: `/mnt/data/soc-system-course/README.md`
- Create: `/mnt/data/soc-system-course/references/README.md`

**Interfaces:**
- Consumes: approved spec.
- Produces: stable course root paths used by all later documents.

- [ ] **Step 1: Create the directory skeleton**

Run:

```bash
mkdir -p /mnt/data/soc-system-course/{00_course_guide,01_templates,02_linux/week01,03_zephyr/week01,04_deep_dive,05_labs/imx6ull,05_labs/stm32f407,06_interview,references}
```

Expected: all directories exist with no error.

- [ ] **Step 2: Write `README.md` with exact first-batch navigation**

Required sections:

```markdown
# SoC System Software Daily Course

## 使用方式
## 第一批内容
## Linux 主线
## Zephyr 主线
## Deep Dive
## References
## 学习产物约定
```

The first-batch content table must contain links to all seven Week 1 tutorials and the DeviceTree deep-dive.

- [ ] **Step 3: Write `references/README.md`**

It must define the expected local reference directory convention:

```text
references/
├── Explorer STM32F4_V2.2_SCH.pdf
├── 【正点原子】I.MX6U嵌入式Linux C应用编程指南V1.1.pdf
└── 【正点原子】I.MX6U嵌入式Linux驱动开发指南V1.5.2.pdf
```

It must state that a `#page=N` fragment is convenience-only and textual page/chapter references remain authoritative.

- [ ] **Step 4: Validate navigation headings**

Run:

```bash
python - <<'PY'
from pathlib import Path
p = Path('/mnt/data/soc-system-course/README.md')
t = p.read_text(encoding='utf-8')
required = ['## 使用方式','## 第一批内容','## Linux 主线','## Zephyr 主线','## Deep Dive','## References','## 学习产物约定']
missing = [x for x in required if x not in t]
assert not missing, missing
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 5: Commit checkpoint**

If working in Git:

```bash
git add README.md references/README.md
git commit -m "docs: create daily course root and reference layout"
```

---

### Task 2: Build course guide and source registry

**Files:**
- Create: `/mnt/data/soc-system-course/00_course_guide/learning_method.md`
- Create: `/mnt/data/soc-system-course/00_course_guide/environment_baseline.md`
- Create: `/mnt/data/soc-system-course/00_course_guide/hardware_inventory.md`
- Create: `/mnt/data/soc-system-course/00_course_guide/source_index.md`
- Create: `/mnt/data/soc-system-course/00_course_guide/glossary.md`

**Interfaces:**
- Consumes: STM32F4 schematic facts, approved spec.
- Produces: stable environment, source IDs, terminology and verified hardware facts referenced by all tutorials.

- [ ] **Step 1: Write `learning_method.md`**

It must define the daily 2-hour execution loop:

```text
15 min recall
35 min theory
55 min experiment/debug
15 min README/log/commit
```

It must include:
- how to mark optional material;
- how to keep lab logs;
- when AI can be used;
- rule that AI does not write the first version of a driver/lab solution;
- weekly review and oral explanation requirements.

- [ ] **Step 2: Write `environment_baseline.md`**

Canonical host:

```text
Windows Host
  └─ VMware Workstation
      └─ Ubuntu 24.04 LTS
```

Document:
- recommended VM CPU/RAM/disk;
- bridged networking for board work;
- snapshot policy;
- workspace layout;
- `python3 -m venv`;
- tool-version capture command;
- cross-toolchain version capture;
- Zephyr workspace activation convention.

Do not require downgrading the main VM for an old vendor BSP; isolate old build dependencies in container/secondary environment if necessary.

- [ ] **Step 3: Write verified `hardware_inventory.md`**

For the Explorer board, record only facts supported by the uploaded schematic, including:

```text
MCU: STM32F407ZET6
HSE: 8 MHz
LSE: 32.768 kHz
USB-UART: CH340G
SPI NOR: W25Q128
Ethernet PHY: LAN8720A
JTAG/SWD interface present
```

Also record the learning role for each resource:
- UART → Zephyr console;
- LED/KEY → board port smoke test;
- W25Q128 → later MCUboot secondary/staging candidate;
- LAN8720 → explicitly deferred in first batch.

- [ ] **Step 4: Write `source_index.md`**

Required IDs:

```text
SRC-F407-SCH
SRC-IMX6ULL-APP
SRC-IMX6ULL-DRV
SRC-NXP-IMX6ULL-RM
SRC-LINUX-DT
SRC-LINUX-DRIVER-MODEL
SRC-ZEPHYR-GETTING-STARTED
SRC-ZEPHYR-DT
SRC-MCUBOOT
```

For `SRC-F407-SCH`, include exact known page roles:
- page 1: Ethernet/audio;
- page 2: MCU/core/JTAG/clock/I/O;
- page 3: peripherals including W25Q128;
- page 4: power/CH340 USB-UART;
- page 5: board placement.

- [ ] **Step 5: Write `glossary.md`**

Minimum terms:
- Host / Target
- BSP
- Toolchain
- Sysroot
- ELF section / segment
- Device Tree / DTS / DTB / FDT
- `struct device_node`
- `struct device`
- `platform_device`
- `platform_driver`
- probe
- Kconfig
- west
- Zephyr binding

Each definition is 1–4 sentences, not a deep-dive.

- [ ] **Step 6: Validate hardware facts**

Run:

```bash
python - <<'PY'
from pathlib import Path
t = Path('/mnt/data/soc-system-course/00_course_guide/hardware_inventory.md').read_text(encoding='utf-8')
for token in ['STM32F407ZET6','W25Q128','LAN8720A','CH340G','8 MHz','32.768']:
    assert token in t, token
for wrong in ['STM32F407ZGT6','1 MB internal Flash']:
    assert wrong not in t, wrong
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 7: Commit checkpoint**

```bash
git add 00_course_guide
git commit -m "docs: add course baseline hardware and source registry"
```

---

### Task 3: Write and validate the reusable daily tutorial template

**Files:**
- Create: `/mnt/data/soc-system-course/01_templates/daily_tutorial_template.md`

**Interfaces:**
- Consumes: course guide conventions.
- Produces: exact section contract for every Daily Tutorial.

- [ ] **Step 1: Write the template with required sections**

The file must include these exact top-level markers:

```markdown
## 0. 今日定位
## 1. 今天解决的工程问题
## 2. 今日能力构成
## 3. 先理解：费曼解释
## 4. 原理
## 5. 结构/机制图
## 6. UML/时序图
## 7. 阅读资料
## 8. 实验准备
## 9. Lab 1
## 11. 故障注入
## 12. 调试路径
## 13. 源码追踪
## 14. 今日验收
## 15. 面试式复述
## 16. Git 交付物
## 17. 明日连接
```

It must explain that sections 6 and 10 can be omitted when semantically unnecessary, while sections 0/1/3/7/8/9/11/12/14/16 cannot be omitted.

- [ ] **Step 2: Add diagram rules with examples**

Include one valid `flowchart` example and one valid `sequenceDiagram` example. Explicitly ban decorative diagrams.

- [ ] **Step 3: Add reference-format examples**

Include one local-source example:

```markdown
- `SRC-F407-SCH`
  - 页码：p.3
  - 阅读目标：确认 W25Q128 与 SPI1 的连接
```

and one official-source example with title + direct link + reading objective.

- [ ] **Step 4: Validate required headings**

Run:

```bash
python - <<'PY'
from pathlib import Path
t = Path('/mnt/data/soc-system-course/01_templates/daily_tutorial_template.md').read_text(encoding='utf-8')
required = [
'## 0. 今日定位','## 1. 今天解决的工程问题','## 2. 今日能力构成',
'## 3. 先理解：费曼解释','## 4. 原理','## 5. 结构/机制图',
'## 7. 阅读资料','## 8. 实验准备','## 9. Lab 1',
'## 11. 故障注入','## 12. 调试路径','## 13. 源码追踪',
'## 14. 今日验收','## 15. 面试式复述','## 16. Git 交付物','## 17. 明日连接'
]
missing=[x for x in required if x not in t]
assert not missing, missing
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 5: Commit checkpoint**

```bash
git add 01_templates/daily_tutorial_template.md
git commit -m "docs: define daily tutorial authoring contract"
```

---

### Task 4: Write W01D01 Ubuntu 主开发环境

**Files:**
- Create: `/mnt/data/soc-system-course/02_linux/week01/W01D01_Ubuntu主开发环境.md`

**Interfaces:**
- Consumes: environment baseline, daily template.
- Produces: a reproducible Ubuntu/VMware environment required by every later Linux and Zephyr task.

- [ ] **Step 1: Write the engineering problem and mental model**

Explain why embedded development needs a stable Host environment before board work.

Mermaid capability diagram must show:

```text
Windows Host → VMware Ubuntu → Toolchain/TFTP/NFS/SSH → Target
```

- [ ] **Step 2: Explain VMware networking with a Feynman model**

Compare:
- NAT;
- bridged;
- host-only.

The conclusion for board development must be:
- bridged is the default when the VM and physical board need to behave as peers on the same LAN;
- NAT can work for Internet access but complicates inbound board→VM access;
- host-only is useful for isolation but not the default course topology.

- [ ] **Step 3: Provide exact Ubuntu setup commands**

Include commands for:
- `apt update`;
- `git`;
- `build-essential`;
- `cmake`;
- `ninja-build`;
- `python3-venv`;
- `openssh-server`;
- `tmux`;
- `tree`;
- `gdb`;
- `gdb-multiarch`;
- `tftp-hpa` client;
- NFS client/server packages.

Every command block must be followed by “what this command changes”.

- [ ] **Step 4: Define workspace and version baseline**

Required workspace:

```text
~/work/
├── src/
├── toolchains/
├── nfs/
├── tftp/
├── logs/
└── course/
```

Version capture must include:

```bash
uname -a
lsb_release -a
gcc --version
cmake --version
python3 --version
git --version
ip addr
ip route
```

- [ ] **Step 5: Add fault injection**

At least:
- disable/stop SSH and diagnose connection failure;
- switch VM from bridged to NAT, observe IP/topology change, then restore.

- [ ] **Step 6: Define Pass/Fail**

Pass requires:
- VM has stable IP;
- Host can SSH into VM;
- VM can access Internet;
- `hello_x86` compiles and runs;
- environment baseline file is saved.

- [ ] **Step 7: Validate content**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/02_linux/week01/W01D01_Ubuntu主开发环境.md').read_text(encoding='utf-8')
for x in ['bridged','NAT','host-only','gdb-multiarch','~/work/','故障注入','今日验收']:
    assert x.lower() in t.lower(), x
assert '```mermaid' in t
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 8: Commit checkpoint**

```bash
git add 02_linux/week01/W01D01_Ubuntu主开发环境.md
git commit -m "docs: add week1 day1 Ubuntu environment tutorial"
```

---

### Task 5: Write W01D02 交叉编译与 ELF 基线

**Files:**
- Create: `/mnt/data/soc-system-course/02_linux/week01/W01D02_交叉编译与ELF基线.md`

**Interfaces:**
- Consumes: working Ubuntu host.
- Produces: cross-toolchain and ELF analysis baseline used in kernel/module and target work.

- [ ] **Step 1: Build the compiler pipeline explanation**

Must explain:

```text
.c → preprocessing → assembly/IR generation → .s → .o → linker → ELF
```

Map this to the user’s existing MCU knowledge:
- startup;
- linker script;
- memory map;
- entry point.

- [ ] **Step 2: Explain Host vs Target vs toolchain triplet**

Use examples:

```text
x86_64-linux-gnu
arm-linux-gnueabihf
```

Explain ABI and sysroot.

- [ ] **Step 3: Provide the x86 vs ARM hello experiment**

Commands must include:
- compile;
- `file`;
- `readelf -h`;
- `readelf -S`;
- `readelf -l`;
- `nm`;
- `objdump -d`.

The tutorial must explicitly explain section vs segment.

- [ ] **Step 4: Add sequence diagram only if semantically used**

Use a sequence diagram to show:
`developer → compiler driver → assembler → linker → ELF loader`, or omit if a static pipeline graph explains it better.

- [ ] **Step 5: Add fault injection**

Run ARM ELF on x86 and explain why `Exec format error` occurs.

- [ ] **Step 6: Define Pass/Fail**

Pass requires the learner to explain without notes:
- section vs segment;
- static vs dynamic link;
- why host cannot execute target ELF;
- what the ELF entry point means.

- [ ] **Step 7: Validate tooling coverage**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/02_linux/week01/W01D02_交叉编译与ELF基线.md').read_text(encoding='utf-8')
for x in ['readelf -h','readelf -S','readelf -l','objdump','nm','Exec format error','sysroot','segment','section']:
    assert x in t, x
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 8: Commit checkpoint**

```bash
git add 02_linux/week01/W01D02_交叉编译与ELF基线.md
git commit -m "docs: add cross compilation and ELF tutorial"
```

---

### Task 6: Write W01D03 6ULL 串口与网络链路

**Files:**
- Create: `/mnt/data/soc-system-course/02_linux/week01/W01D03_6ULL串口与网络链路.md`

**Interfaces:**
- Consumes: Ubuntu host/network baseline.
- Produces: verified serial and Ethernet control paths to the 6ULL.

- [ ] **Step 1: Explain the two control planes**

Distinguish:
- U-Boot serial console;
- Linux serial console;
- Linux Ethernet/SSH.

Use a static architecture diagram.

- [ ] **Step 2: Explain minimum network theory for BSP work**

Cover only:
- IP/subnet;
- ARP;
- default route;
- same-LAN peer model;
- why VM bridged mode matters.

Do not turn this day into a full routing/networking course.

- [ ] **Step 3: Give the serial procedure**

The tutorial must instruct the learner to:
- identify USB serial device;
- set baud/data/parity/stop;
- reset board;
- capture U-Boot log;
- stop autoboot;
- continue to Linux;
- save full boot log.

If exact board baud rate cannot be verified from source material, explicitly say to use the vendor guide/default actually supplied with the board instead of inventing it.

- [ ] **Step 4: Give the board networking procedure**

Include:
- inspect `ip addr`;
- set temporary static IP;
- ping VM;
- check `ip neigh`;
- verify SSH;
- copy a file.

- [ ] **Step 5: Add fault injection**

At least:
- wrong subnet mask;
- duplicate/incorrect IP;
- VM firewall or wrong interface.

- [ ] **Step 6: Define Pass/Fail**

Pass:
- serial console reaches U-Boot and Linux;
- board and VM ping both directions;
- learner can explain ARP vs route at this topology.

- [ ] **Step 7: Validate content**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/02_linux/week01/W01D03_6ULL串口与网络链路.md').read_text(encoding='utf-8')
for x in ['U-Boot','Linux console','ARP','ip neigh','ip route','SSH','故障注入']:
    assert x in t, x
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 8: Commit checkpoint**

```bash
git add 02_linux/week01/W01D03_6ULL串口与网络链路.md
git commit -m "docs: add imx6ull serial and network tutorial"
```

---

### Task 7: Write W01D04 TFTP 与 NFS 开发闭环

**Files:**
- Create: `/mnt/data/soc-system-course/02_linux/week01/W01D04_TFTP与NFS开发闭环.md`

**Interfaces:**
- Consumes: verified VM↔board Ethernet and U-Boot console.
- Produces: rapid BSP development loop for later kernel/DTB/app experiments.

- [ ] **Step 1: Explain why TFTP and NFS solve different phases**

Required mental model:

```text
U-Boot phase: TFTP fetches kernel/DTB/test files
Linux phase: NFS exposes host files/rootfs
```

Compare with SCP.

- [ ] **Step 2: Configure TFTP**

Document:
- package/service;
- root directory;
- permissions;
- service status;
- host-side test;
- U-Boot environment variables to inspect before use.

Do not invent specific `loadaddr` or `fdt_addr` values: require reading the board’s existing environment first.

- [ ] **Step 3: Configure NFS**

Include:
- `/etc/exports`;
- export path;
- `exportfs -ra`;
- service status;
- target mount command;
- read/write test.

- [ ] **Step 4: Add a practical closed-loop experiment**

Sequence:
1. edit host file;
2. target reads through NFS;
3. modify target-side test program;
4. cross-compile;
5. run from NFS.

- [ ] **Step 5: Add fault injection**

Minimum:
- wrong TFTP server IP;
- wrong export;
- NFS permission;
- firewall/service stopped.

- [ ] **Step 6: Define Pass/Fail**

Pass:
- U-Boot can retrieve a known small test file via TFTP;
- Linux can mount the NFS path;
- host edit is immediately visible to target.

- [ ] **Step 7: Validate content**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/02_linux/week01/W01D04_TFTP与NFS开发闭环.md').read_text(encoding='utf-8')
for x in ['TFTP','NFS','/etc/exports','exportfs -ra','U-Boot','SCP','故障注入']:
    assert x in t, x
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 8: Commit checkpoint**

```bash
git add 02_linux/week01/W01D04_TFTP与NFS开发闭环.md
git commit -m "docs: add tftp nfs bsp development loop tutorial"
```

---

### Task 8: Write W01D05 Zephyr 官方环境

**Files:**
- Create: `/mnt/data/soc-system-course/03_zephyr/week01/W01D05_Zephyr官方环境.md`

**Interfaces:**
- Consumes: Ubuntu host.
- Produces: reproducible Zephyr workspace and official-board baseline before custom F407 board port.

- [ ] **Step 1: Explain the Zephyr workspace model**

Must explain:
- Python venv;
- west;
- workspace;
- manifest;
- modules;
- Zephyr SDK;
- CMake/Ninja;
- Kconfig;
- Devicetree.

Use a Mermaid structure diagram.

- [ ] **Step 2: Provide the environment setup**

Use official current Zephyr Getting Started flow as basis. The tutorial must pin the actual Zephyr version/commit used during implementation rather than saying “latest”.

- [ ] **Step 3: Build an upstream STM32F4 target**

Use `stm32f4_disco` as the initial target unless official documentation at execution time names a different canonical board.

Required command form:

```bash
west build -b stm32f4_disco samples/hello_world
```

- [ ] **Step 4: Inspect build artifacts**

Require reading:
- `build/zephyr/zephyr.elf`;
- `zephyr.bin`;
- `zephyr.hex`;
- `zephyr.dts`;
- `.config`;
- generated headers.

Explain what each proves.

- [ ] **Step 5: Add fault injection**

At least:
- deactivate venv and demonstrate missing/wrong tool context;
- wrong board name and interpret west/CMake error.

- [ ] **Step 6: Define Pass/Fail**

Pass:
- fresh shell can activate environment;
- `west topdir` works;
- `west build` works;
- learner can locate final DTS and Kconfig outputs.

- [ ] **Step 7: Validate content**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/03_zephyr/week01/W01D05_Zephyr官方环境.md').read_text(encoding='utf-8')
for x in ['python','venv','west','manifest','Zephyr SDK','Kconfig','zephyr.dts','.config','stm32f4_disco']:
    assert x.lower() in t.lower(), x
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 8: Commit checkpoint**

```bash
git add 03_zephyr/week01/W01D05_Zephyr官方环境.md
git commit -m "docs: add Zephyr official environment tutorial"
```

---

### Task 9: Write W01D06 F407 硬件审计与 Board Port 准备

**Files:**
- Create: `/mnt/data/soc-system-course/03_zephyr/week01/W01D06_F407硬件审计与BoardPort准备.md`
- Create: `/mnt/data/soc-system-course/05_labs/stm32f407/f407_board_audit_template.md`

**Interfaces:**
- Consumes: uploaded schematic and Zephyr upstream `stm32f4_disco` baseline.
- Produces: verified board-port input table for later Zephyr custom-board implementation.

- [ ] **Step 1: Explain why board port begins with hardware audit**

Use a Feynman model:
“Zephyr already knows the STM32F407 SoC; the custom board file tells it how this specific PCB wires clock, console, LED, key, flash and peripherals.”

Then explain exact engineering separation:
- SoC support;
- board DTS;
- pinctrl;
- Kconfig;
- defconfig;
- bindings/drivers.

- [ ] **Step 2: Extract exact schematic facts**

The tutorial must cite:
- page 2 for `STM32F407ZET6`, HSE/LSE, JTAG, USART1;
- page 3 for W25Q128/SPI1, LED/KEY and peripheral resources;
- page 4 for CH340G USB-UART;
- page 1 for LAN8720A.

It must explicitly state that the on-board USB-UART is CH340G-side and the MCU connection must be traced via USART1 nets before setting Zephyr console.

- [ ] **Step 3: Create the board-audit table**

The template must have columns:

```text
Resource | Schematic Page | Net | MCU Pin | AF/Mode | Zephyr Node | Driver | Verification
```

Rows minimum:
- HSE;
- LSE;
- USART1 TX;
- USART1 RX;
- LED0;
- LED1;
- KEY0;
- KEY1;
- W25Q128 CS/SCK/MISO/MOSI;
- JTAG/SWD;
- LAN8720 RMII reference.

- [ ] **Step 4: Compare against `stm32f4_disco`**

Identify what can be reused:
- SoC DTSI;
- clock-controller model;
- STM32 pinctrl binding patterns.

Identify what cannot be copied blindly:
- oscillator frequency if different;
- LED/key pins;
- console UART;
- external flash;
- board aliases/chosen.

- [ ] **Step 5: Add fault-prevention exercise**

Ask learner to deliberately compare one pin from a tutorial for a different STM32F407 board and show why board-level copying without schematic verification is unsafe.

- [ ] **Step 6: Define Pass/Fail**

Pass:
- board audit table completed;
- each console/LED/flash pin has a schematic source;
- learner can state the difference between SoC support and board support.

- [ ] **Step 7: Validate hardware identifiers**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/03_zephyr/week01/W01D06_F407硬件审计与BoardPort准备.md').read_text(encoding='utf-8')
for x in ['STM32F407ZET6','W25Q128','CH340G','LAN8720A','USART1','HSE','LSE','stm32f4_disco']:
    assert x in t, x
assert 'STM32F407ZGT6' not in t
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 8: Commit checkpoint**

```bash
git add 03_zephyr/week01/W01D06_F407硬件审计与BoardPort准备.md 05_labs/stm32f407/f407_board_audit_template.md
git commit -m "docs: add F407 hardware audit and Zephyr board port preparation"
```

---

### Task 10: Write W01D07 Week 1 环境复现与复盘

**Files:**
- Create: `/mnt/data/soc-system-course/02_linux/week01/W01D07_Week1复盘与环境复现.md`

**Interfaces:**
- Consumes: outputs of Days 1–6.
- Produces: reproducibility proof and Week 1 readiness gate.

- [ ] **Step 1: Define the cold-start exercise**

Learner must close terminals/restart VM or start from a fresh shell and reproduce:
- host environment;
- cross-toolchain;
- board network;
- Zephyr venv;
- west workspace.

- [ ] **Step 2: Define the build replay**

Must rebuild:
- x86 hello;
- ARM hello;
- Zephyr `hello_world`.

Must verify:
- TFTP small-file transfer;
- NFS mount;
- SSH board access.

- [ ] **Step 3: Define the Week 1 oral questions**

Minimum 12:
- Host vs Target?
- Why bridged?
- Toolchain vs sysroot?
- section vs segment?
- U-Boot console vs Linux console?
- TFTP vs NFS?
- west workspace?
- Kconfig vs DTS?
- SoC support vs board support?
- why cannot copy another F407 board DTS blindly?
- where is W25Q128 on this board?
- what is the Week 2 entry condition?

- [ ] **Step 4: Add explicit Week 1 pass gate**

Week 2 is blocked unless all are true:
- Ubuntu environment reproducible;
- board serial available;
- board networking works;
- TFTP/NFS work;
- ARM toolchain works;
- Zephyr upstream F4 sample builds;
- F407 audit sheet exists.

- [ ] **Step 5: Validate replay checklist**

Run:

```bash
python - <<'PY'
from pathlib import Path
t=Path('/mnt/data/soc-system-course/02_linux/week01/W01D07_Week1复盘与环境复现.md').read_text(encoding='utf-8')
for x in ['TFTP','NFS','Zephyr','west','ARM','F407','Week 2','Pass']:
    assert x.lower() in t.lower(), x
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 6: Commit checkpoint**

```bash
git add 02_linux/week01/W01D07_Week1复盘与环境复现.md
git commit -m "docs: add week1 reproducibility gate"
```

---

### Task 11: Write `A01_DeviceTree_从DTS到LinuxDevice.md`

**Files:**
- Create: `/mnt/data/soc-system-course/04_deep_dive/A01_DeviceTree_从DTS到LinuxDevice.md`

**Interfaces:**
- Consumes: Linux/Zephyr official DeviceTree documentation, 6ULL BSP examples, course terminology.
- Produces: the authoritative DeviceTree mechanism explanation referenced by later driver tutorials.

- [ ] **Step 1: Write the motivation chapter**

Explain:
- board files;
- why hardware description was separated from kernel C code;
- what DT describes;
- what DT does not describe.

Feynman model:
“DT is the hardware inventory and wiring map, not the driver itself.”

- [ ] **Step 2: Write the syntax chapter**

Cover with concrete examples:
- root;
- node;
- unit-address;
- property;
- label;
- phandle;
- `.dts/.dtsi`;
- overlay;
- `compatible`;
- `status`;
- `reg`;
- `ranges`;
- `#address-cells`;
- `#size-cells`;
- interrupts;
- clocks;
- resets;
- pinctrl;
- GPIO specifiers.

For each major property, answer:
1. what hardware fact it expresses;
2. who consumes it;
3. how a driver/subsystem gets it;
4. failure mode if wrong.

- [ ] **Step 3: Explain DTS → DTB**

Include commands:

```bash
dtc -I dts -O dtb
dtc -I dtb -O dts
fdtdump
```

Use a Mermaid pipeline.

- [ ] **Step 4: Explain U-Boot handoff**

Show:
- kernel;
- optional initrd;
- DTB address;
- `bootz/bootm`;
- `fdt addr`;
- `fdt print`.

Do not invent 6ULL addresses; instruct the learner to inspect existing environment.

- [ ] **Step 5: Explain FDT → `struct device_node`**

Must include:

```text
Flat DTB
→ early DT scan
→ unflatten_device_tree()
→ in-memory device_node tree
```

Explicitly say:
“a `device_node` is not yet automatically a Linux `struct device`.”

- [ ] **Step 6: Explain device population**

Required flow:

```text
device_node
→ of_platform_populate()
→ platform_device
→ embedded struct device
```

Include exact structural relation:

```c
struct platform_device {
    ...
    struct device dev;
};
```

Explain the role of the Driver Model.

- [ ] **Step 7: Compare bus-specific device objects**

Create a table:

| Hardware class | Discovery/population | Linux object | Common driver type |
|---|---|---|---|
| SoC MMIO | DT population | `platform_device` | `platform_driver` |
| I2C child | I2C core parses child nodes | `i2c_client` | `i2c_driver` |
| SPI child | SPI core parses child nodes | `spi_device` | `spi_driver` |
| PCIe EP | hardware enumeration | `pci_dev` | `pci_driver` |

Explain that each wrapper embeds or participates in the common `struct device` model.

- [ ] **Step 8: Explain match and probe with a UML sequence**

Sequence participants:
- DT;
- OF/platform population;
- `platform_device`;
- platform bus;
- `platform_driver`.

Must show:
- driver registration;
- bus matching;
- `of_match_table`;
- `probe()`.

- [ ] **Step 9: Explain bindings and vendor differences**

Must distinguish:

```text
DTS grammar
≠
binding schema
≠
driver implementation
```

Explain why different vendor DTS files look different while still sharing common syntax/model:
- different hardware topology;
- vendor compatible strings;
- vendor-specific binding properties;
- subsystem common properties.

- [ ] **Step 10: Compare Linux and Zephyr DeviceTree**

Required contrast:

Linux:
```text
DTS → DTB → boot → runtime parse → device_node → bus object → driver match
```

Zephyr:
```text
DTS + YAML bindings → build processing → generated headers → DT_* macros → DEVICE_DT_* instances
```

Must explicitly state that Zephyr is primarily build-time Devicetree consumption rather than Linux-style runtime DTB device population.

- [ ] **Step 11: Add four practical labs**

Lab A: trace a 6ULL UART from DTS to compatible → driver → probe.

Lab B: change a safe device `status = "disabled"` and observe:
- DTB;
- dmesg;
- `/sys/bus/platform`;
- device presence.

Lab C: create minimal `student,mydev` platform device + driver to observe match/probe.

Lab D: trace a Zephyr LED alias/binding/generated macro/device handle path.

- [ ] **Step 12: Add troubleshooting matrix**

Minimum cases:
- syntax error;
- DT compiles but driver does not probe;
- wrong `compatible`;
- `status = "disabled"`;
- missing clock;
- wrong pinctrl;
- bad `reg`;
- bad interrupt specifier;
- device exists but user-space node does not.

- [ ] **Step 13: Add interview/teach-back section**

Minimum questions:
1. Why does DT exist?
2. Is every DT node a `platform_device`?
3. What is the relationship between `device_node` and `struct device`?
4. Who calls `probe()`?
5. What exactly matches `compatible`?
6. Why is an I2C sensor an `i2c_client` rather than `platform_device`?
7. What does binding mean?
8. Why can vendor DTS syntax look different?
9. Linux DT vs Zephyr DT?
10. How do you trace a node to its driver?

- [ ] **Step 14: Validate required mechanism tokens and diagrams**

Run:

```bash
python - <<'PY'
from pathlib import Path
p=Path('/mnt/data/soc-system-course/04_deep_dive/A01_DeviceTree_从DTS到LinuxDevice.md')
t=p.read_text(encoding='utf-8')
required=[
'DTS','DTB','FDT','struct device_node','unflatten_device_tree',
'of_platform_populate','struct platform_device','struct device',
'i2c_client','spi_device','pci_dev','of_match_table','probe',
'YAML','DEVICE_DT_','binding'
]
missing=[x for x in required if x not in t]
assert not missing, missing
assert t.count('```mermaid') >= 3
assert 'sequenceDiagram' in t
print('PASS')
PY
```

Expected: `PASS`.

- [ ] **Step 15: Commit checkpoint**

```bash
git add 04_deep_dive/A01_DeviceTree_从DTS到LinuxDevice.md
git commit -m "docs: add DeviceTree from dts to Linux device deep dive"
```

---

### Task 12: Cross-document source/link consistency pass

**Files:**
- Modify: all files under `/mnt/data/soc-system-course/`

**Interfaces:**
- Consumes: Tasks 1–11.
- Produces: internally consistent first-batch tutorial set.

- [ ] **Step 1: Verify every source ID is registered**

Run:

```bash
python - <<'PY'
from pathlib import Path
root=Path('/mnt/data/soc-system-course')
idx=(root/'00_course_guide/source_index.md').read_text(encoding='utf-8')
ids=set()
for line in idx.splitlines():
    line=line.strip()
    if line.startswith('SRC-'):
        ids.add(line.split()[0].strip('`'))
used=set()
for p in root.rglob('*.md'):
    if p.name=='source_index.md':
        continue
    for token in p.read_text(encoding='utf-8').replace('`',' ').split():
        if token.startswith('SRC-'):
            used.add(token.strip('.,:;()[]'))
unknown=used-ids
assert not unknown, sorted(unknown)
print('PASS', len(ids), 'registered IDs')
PY
```

Expected: `PASS`.

- [ ] **Step 2: Check internal Markdown links**

Use a small Python validator that checks relative `.md` links point to existing files. Do not fail on external HTTP links or local PDFs that may not yet be copied into `references/`.

- [ ] **Step 3: Check prohibited hardware assumptions**

Run:

```bash
grep -R "STM32F407ZGT6\|1 MB internal Flash" /mnt/data/soc-system-course && exit 1 || true
```

Expected: no matches.

- [ ] **Step 4: Verify each Daily Tutorial contains the mandatory sections**

Use a Python check over the seven W01 files for:
- `今日定位`
- `今天解决的工程问题`
- `先理解：费曼解释`
- `阅读资料`
- `实验准备`
- `Lab`
- `故障注入`
- `调试路径`
- `今日验收`
- `Git 交付物`

Expected: all seven pass.

- [ ] **Step 5: Commit checkpoint**

```bash
git add .
git commit -m "docs: normalize first-batch tutorial references and links"
```

---

### Task 13: Semantic quality review against the approved spec

**Files:**
- Review only; modify any failing file.

**Interfaces:**
- Consumes: entire first batch.
- Produces: reviewed deliverable ready for user inspection.

- [ ] **Step 1: Coverage review**

Create an internal checklist mapping:
- Spec §3 directory structure → created files.
- Spec §4 daily template → `daily_tutorial_template.md`.
- Spec §5 diagrams → every relevant day.
- Spec §6 Feynman explanation → difficult concepts.
- Spec §7 DeviceTree → A01 sections/labs.
- Spec §8 Week 1 scope → D01–D07.
- Spec §9 reference convention → `source_index.md` and each day.
- Spec §11 quality gate → review results.

Fix any unmapped requirement before proceeding.

- [ ] **Step 2: Check daily scope against 2-hour budget**

For each day, mark content into:
- Core 2h;
- Optional extension.

Any experiment that realistically requires environment download/installation time must note that wall-clock download time is outside active study time or be split clearly.

- [ ] **Step 3: Check Mermaid semantics**

For every Mermaid block:
- capability diagrams answer “where does this skill sit?”;
- structure diagrams answer “what relates to what?”;
- sequence diagrams contain real temporal/participant semantics.

Delete decorative diagrams.

- [ ] **Step 4: Hardware reality check**

Compare W01D06 and `hardware_inventory.md` to the uploaded schematic:
- MCU model;
- clock;
- CH340;
- W25Q128;
- LAN8720;
- page references.

Fix any discrepancy.

- [ ] **Step 5: DeviceTree teach-back check**

The A01 deep-dive passes only if a reader can answer, from the document alone:

```text
Why DT exists
DTS vs DTB
What unflattening produces
Why not every node becomes platform_device
How platform_device embeds struct device
How I2C/SPI/PCI device objects differ
Who matches compatible
Who calls probe
What bindings constrain
Why Linux and Zephyr DT usage differ
```

- [ ] **Step 6: Final placeholder scan**

Run:

```bash
grep -RniE '\b(TBD|TODO|FIXME)\b|以后补|待补|这里略' /mnt/data/soc-system-course
```

Expected: no unresolved authoring placeholders. A quoted source/code occurrence is allowed only if explicitly marked as source content.

- [ ] **Step 7: Commit review fixes**

```bash
git add .
git commit -m "docs: complete first-batch tutorial quality review"
```

---

### Task 14: Package first-batch deliverables for user review

**Files:**
- Create: `/mnt/data/soc-system-course-first-batch.zip`
- Create: `/mnt/data/soc-system-course/00_course_guide/first_batch_review_checklist.md`

**Interfaces:**
- Consumes: reviewed first batch.
- Produces: one downloadable package plus a focused user-review checklist.

- [ ] **Step 1: Write the review checklist**

The checklist must ask the user to assess:

1. Is W01D01 a real tutorial rather than an outline?
2. Is the core daily workload realistic for 2 hours?
3. Are local PDF references usable?
4. Are diagrams actually explanatory?
5. Does DeviceTree finally make `DTS → Linux device object → match/probe` clear?
6. Is the Linux-vs-Zephyr DT comparison clear?
7. Is any section too verbose or too shallow?

- [ ] **Step 2: Create zip**

Run:

```bash
cd /mnt/data
zip -r soc-system-course-first-batch.zip soc-system-course
```

Expected: archive created successfully.

- [ ] **Step 3: Verify archive**

Run:

```bash
unzip -t /mnt/data/soc-system-course-first-batch.zip
```

Expected: `No errors detected`.

- [ ] **Step 4: Final deliverable report**

Report:
- total Markdown file count;
- first-batch archive path;
- direct links to W01D01 and DeviceTree deep-dive;
- known external dependencies still requiring user local PDFs.

---

# Self-Review Results

## Spec coverage
All approved spec areas are mapped:
- course layout → Tasks 1–3;
- Week 1 seven tutorials → Tasks 4–10;
- DeviceTree deep-dive → Task 11;
- source/link/hardware correctness → Tasks 12–13;
- first-batch user review gate → Task 14.

## Placeholder scan
The implementation plan contains no unresolved `TBD`, `TODO`, or “similar to Task N” steps.

## Interface consistency
Stable paths and source IDs are defined before they are consumed. Later tasks refer only to files created in earlier tasks or to external/user-provided source material.

## Scope check
The first batch is intentionally bounded to Week 1 + DeviceTree. Zephyr MCUboot/OTA implementation, Linux driver weeks, and AI Runtime remain outside this implementation plan.
