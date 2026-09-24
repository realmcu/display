# DCI 应用层功能测试 demo

裸机环境，无 OS。

本工程的被测对象是 **DCI/DCMI 接收模块本身**。摄像头侧按
`dci_8773g/note/preset-table.md` 配置去产生各种激励信号，本机这张表负责**把它们全部收下来**。

**一次编译只跑一个 case。** 三项选择都在 `dci_demo_config.h` 里，编译期固定；
只有被选中的 case 参与编译，也只有它的 buffer 会占 RAM。

一次测试 = 三个相互独立的选择：

| | 选什么 | 宏 |
|---|---|---|
| 1 | **case** —— 跑什么流程 | `DCI_DEMO_CASE` |
| 2 | **signal** —— 引脚上来的是什么（须与摄像头侧一致） | `DCI_DEMO_SIGNAL` |
| 3 | **overlay** —— 本机收下来之后再做什么：crop / select / 降帧 | `DCI_DEMO_OV_*` |

三者相乘：**3 case × 13 signal × overlay**，只改几行配置，不写新代码。

**case 只有 3 个**，因为 case 应该是"不同的流程"，不是"同一个流程换个数字" ——
后者是宏。`snapshot-x2` / `continuous-1buf` / `stress` / `config-only` / `receive`
现在都是宏组合，详见 [TESTMATRIX.md §1](TESTMATRIX.md#一case-表)。
非法组合会被拒绝，并**指名该改哪个宏** —— 能在编译期判的在 `dci_demo_config.h` 判，
需要查表才知道的（如「2 字节格式上开 byte select」）由 `dci_demo_cfg_resolve()` 判。

设计与实现见 [../DESIGN.md](../DESIGN.md)，官方用例整理见 [../TESTPLAN.md](../TESTPLAN.md)，
**速查表见 [TESTMATRIX.md](TESTMATRIX.md)** —— 每行都注明测试用意。

---

## 为什么是"一次一个 case"

不是限制，是三个具体的收益：

1. **RAM。** 总预算 1 MB，framebuffer 池占掉大部分。函数指针表的做法会把所有
   case 都链进来，池必须按**最坏**的 case 来定；编译期选择则按**当前**的 case 来定。
   在 1 MB 下这个差别直接决定最大的几行 signal 能不能跑。
2. **不会跑错。** 烧进去的就是要跑的，没有第二条路径。
3. **半成品不阻塞。** 没选中的 case 不编译，改坏一个 case 不会影响正在测的那个。

换测试项 = 改 `dci_demo_config.h` → 重编 → 重烧。

## 文件

| 文件 | 内容 |
|---|---|
| **`dci_demo_config.h`** | **唯一需要改的文件**：case、signal、overlay、buffer 数与填充方式、中断开关、捕获第几帧、RAM 预算 |
| `dci_demo_id.h` | case / signal / crop / 填充方式编号（普通 `#define`，因为要在 `#if` 里比较，枚举不行） |
| `dci_demo.h` | 公共接口：`dci_demo_run(void)`、结果码、中断计数结构 |
| `dci_demo.c` | 编译期 case 选择（`#if/#elif` 链）、**共用 setup**、**环境初始化**、banner、统计断言 |
| `dci_demo_case.c` | 3 个 case 的线性脚本，每个都包在 `#if DEMO_CASE_IS(...)` 里 |
| `dci_demo_signal.{h,c}` | **唯一配置源**：13 行 signal 表 + overlay + 几何推导 + 合法性规则 + 配置打印 |
| `dci_demo_irq.c` | 五个中断源的计数与逐帧报告、「保留第 N 帧」的实现 |
| `dci_demo_buf.{h,c}` | framebuffer 池、哨兵、poison、分级 dump、CRC32、JPEG 结构校验、**10-bit 逐样本校验**、**JLink 地址范围打印** |

## 用法

`dci_demo_config.h` 里改几行：

```c
#define DCI_DEMO_CASE           DCI_DEMO_CASE_SNAPSHOT
#define DCI_DEMO_SIGNAL         DCI_DEMO_SIG_QQVGA_RGB565  /* 160x120，DTCM 装得下 */
#define DCI_DEMO_OV_CROP        DCI_DEMO_CROP_OFF          /* overlay 全部默认关 */
#define DCI_DEMO_BUF_COUNT      1
#define DCI_DEMO_FILL_MODE      DCI_DEMO_FILL_GATE
#define DCI_DEMO_SHOTS          1       /* 2 = 双缓冲轮转检查，需 BUF_COUNT=2 + 80 KB 池 */
#define DCI_DEMO_NO_CAPTURE     0       /* 1 = 只配置不采集，不需摄像头 */
#define DCI_DEMO_POOL_KB        40      /* 见下方 RAM 一节，需配 scatter 改动 */
```

`main()` 里一行：

```c
#include "dci_demo.h"

int main(void)
{
    /* ... 时钟与 log UART 就绪 ... */

    dci_demo_run();          /* 跑 dci_demo_config.h 选定的那个 case */
}
```

### 环境初始化 `dci_demo_env_init()`

`dci_demo_run()` **开头会自动调它**，所以上面那一行就够了。它做的事等价于上板时手动跑的
JLink 序列：

```
w4 0x40701080, 0xFFFFFFFF
w4 0x40701084, 0xFFFFFFFF
w4 0x40701088, 0xFFFFFFFF
w4 0x4070108C, 0xFFFFFFFF
w4 0x40701090, 0xFFFFFFFF
```

五个连续 32-bit 字全写 1。**这不属于 DCI 驱动** —— 驱动只配 DCI 自己，这五个字是
DCI 所处平台的开门动作，所以放在 demo 里（`dci_demo.c`），
应用完全可以用自己的方式做同样的事而不调它。

自己驱动 case、不走 `dci_demo_run()` 的应用需要**先手动调一次**：

```c
dci_demo_env_init();         /* 必须在任何 DCI 寄存器访问之前 */
```

重复写同一个全 1 值没有副作用，所以两处都调也没问题。

> 目前刻意用**字面地址**而不是命名寄存器：这个块的寄存器表还没见到，
> 起名字等于断言了不知道的东西。表拿到后再命名、并写清每一位的作用。

所有宏都用 `#ifndef` 包着，所以 CI 可以 `-DDCI_DEMO_CASE=...` 覆盖，不必改文件 ——
本地的主机侧扫描就是这么把 3 个 case、13 行 signal 和各种 overlay 全编一遍的。

**开始接收数据前，配置会被完整打印出来。** 先是 banner（case、buffer 数、填充方式、
中断开关），紧接着是 `dci_demo_cfg_print()` 的完整配置记录：

```
================ DCI demo ================
 case   : 1  snapshot   [needs a live camera]
 buffers: 2
 fill   : GATE - stops rather than overwrite a held buffer
 irq    : FRAME/OVR/ERR on (always), VSYNC off, LINE off, per-frame log off
 shots  : 1
 capture: yes - frame held on delivery, address printed for JLink
==========================================

---------------- configuration ----------------
 signal row   : 2  qvga-rgb565
 stimulus     : qvga-rgb565 (also -yuv422 / -colorbar / -pclk-gated)
 ...
```

选择是编译期的，所以**旧版本的 log 和新版本的 log 长得一样** —— 这段输出就是唯一
能区分它们的东西，不是装饰。打印只有一处（`dci_demo_cfg_print()`），
所以**打出来的和下发给硬件的不可能对不上**。

---

## 测试项

**3 个 case。**

| # | case | 需要摄像头 | 作用 |
|---|---|---|---|
| 0 | `reg-test` | ❌ | 寄存器读写自检。**第一个跑** |
| 1 | **`snapshot`** | ✅¹ | **单帧，全部校验，打印地址范围供 JLink dump** |
| 2 | `continuous` | ✅ | `DCI_DEMO_FRAMES` 帧、闭环、背压 |

¹ 配 `DCI_DEMO_NO_CAPTURE=1` 时不需要摄像头。

**0 号完全不需要摄像头**，1 号配 `NO_CAPTURE=1` 也不需要 —— **两者今天就能跑**：
DCI 时钟默认开启，寄存器块直接可达（DESIGN.md §17 项 12）。

> **FPGA 阶段驱动侧已无缺口**：DMA 握手索引 = 48（AP DMA1），DCI 时钟默认开，
> **引脚不需要配置** —— DCI 在 FPGA 上是顶层专用 RTL 端口、由 `.xdc` 钉死，
> 不经过 SoC pinmux（DESIGN.md §17 项 13）。
> 另注 DCI **只能跑在 DMA1 通道 16~23** 上 —— AP DMA0 已从硅片裁掉，驱动自动强制。

### RAM：池放在 DTCM 的独立段（默认已按 160×120 配好）

默认值就是 RTL87X3K AP `ram_proj` 的上板配置：**池 40 KB，放在自己的
`.dci_pool` 段**，够 160×120 RGB565 单缓冲（实测需 38 432 B）。

**需要在 SDK 侧改两个文件**，否则链接器找不到这个段：

**① `soc/ram_proj/keil_proj/ap/inc/mem_config.h`** —— 从 heap 前面切出 40 KB：

```c
#define DATA_RAM_VECTOR_TABLE_SIZE              (10*1024)

/* DCI 测试池。160x120 RGB565 单缓冲需 38432 B；双缓冲要 (80*1024) */
#define DCI_POOL_SIZE                           (40*1024)
#define DCI_POOL_ADDR                           (DATA_RAM_VECTOR_TABLE_ADDR + DATA_RAM_VECTOR_TABLE_SIZE)

/* -----------------The following macros should not be modified!--------------- */
#define DATA_RAM_VECTOR_TABLE_ADDR           (DATA_RAM_ADDR)
#define DATA_HEAP_ADDR                       (DCI_POOL_ADDR + DCI_POOL_SIZE)   /* 改这行 */
#define DATA_HEAP_SIZE                       (DATA_RAM_ADDR + DATA_RAM_SIZE - DATA_HEAP_ADDR)
```

`DATA_HEAP_SIZE` 是算出来的，会自动缩（118 KB → 78 KB）。

**② `app_rom.sct`** —— 在 `RW_SECTION` 的 `ScatterAssert` 之后、`ARM_LIB_HEAP` 之前插入：

```
	ScatterAssert((ImageLength(RAM_VECTOR_TABLE) + ImageLength(RW_SECTION)) < RW_SECTION_SIZE)

    DCI_POOL DCI_POOL_ADDR DCI_POOL_SIZE
    {
        * (.dci_pool)
    }
```

> **必须是独立执行域。** 若落进 `RW_SECTION`，那条 10 KB 的 `ScatterAssert` 会照样拦：
> ```
> L6388E: ScatterAssert ... failed : (0xf1580 < 0x2800)
> ```
> `app_ram.sct` 没有 heap 域，那边只加 `DCI_POOL` 域即可，不用改 heap。

改完的 DTCM 布局：

```
0x20000000  ┬ RAM_VECTOR_TABLE + RW_SECTION   10 KB   (上限不变)
0x20002800  ┼ DCI_POOL                        40 KB   ← 新增
0x2000C800  ┼ ARM_LIB_HEAP                    78 KB   (原 118 KB)
0x20020000  ┴
```

**heap 从 118 KB 降到 78 KB** —— 裸机测试工程一般够用，但若别处 `malloc` 较多要留意。

#### 想跑更大的行

DTCM 总共只有 128 KB，所以大尺寸必须换区域。各行实测需求：

| signal | 单缓冲 | 双缓冲 |
|---|---|---|
| `null-min`、`qqvga-rgb565`（160×120） | **38 KB** | 76 KB |
| `qvga-jpeg`（64 KB 上限） | 65 KB | 131 KB |
| `qvga-raw8` | 76 KB | 150 KB |
| `qvga-rgb565` 及同尺寸各行（320×240） | 150 KB | 300 KB |
| `466-raw8` | 212 KB | 424 KB |
| `vga-rgb565`（640×480） | 600 KB | 1200 KB |

改法就是把 `DCI_POOL` 域指到别的区，并同步 `DCI_DEMO_POOL_KB` / `DCI_DEMO_RAM_BUDGET_KB`。
候选（**需与你们的内存规划确认是否空闲**）：`DSP_SHM` 448 KB、`DATA_SRAM0` 128 KB，
或 `mem_config.h` 里注释掉的 `BUFFER_RAM`。

池不够时**不会静默截断**，会报 `DEMO_FAIL_POOL` 并打出确切差额：

```
 pool need    : 153632 B of 40960 B  (budget 40 KB)
  pool too small: need 153632 B for 1 x 153600 B, have 40960 B
======== snapshot: FAIL (POOL) ========
```

> `REG_TEST` 和 `SNAPSHOT + NO_CAPTURE=1` **根本不碰池**。若暂时不想改 scatter，
> 用 `-DDCI_DEMO_SECTION=` `-DDCI_DEMO_POOL_KB=1 -DDCI_DEMO_RAM_BUDGET_KB=1`
> 把池清空并退回默认 RW 段，这两个 case 立刻能链。

### FPGA 上板前提（板级，非驱动配置）

| 项 | 要求 |
|---|---|
| **bitfile** | 必须是**带 DCMI 的**那一版。DCI 脚位在 J4 上复用了 MIPI D-PHY 调试信号与 LCDC 数据线 → **DCMI 与 MIPI/LCDC 在该连接器上互斥** |
| **连接器** | J4。`dcmi_pixclk` = FPGA `BB9`（J4 `G5`，clock-capable 脚）；`dcmi_hsync` = `AU23`（`G26`）；`dcmi_vsync` = `AT22`（`G23`）；`dcmi_d[9:0]` 见引脚表 |
| **电平** | 全部 LVCMOS18（1.8 V），无 pull-up/pull-down 要求 |
| **camera I2C** | `FPGA_IO_PIN_AP[80]`/`[81]`（J4 `G29`/`G30`），刻意排在 DCI 脚位旁供 sensor 初始化 |
| **未用数据线** | 引脚表布了 `dcmi_d[13:0]` 共 14 根，但 **DCI 只有 D[9:0] 十根**（spec 明确）。按 spec「使用位数少于 10 则**必须把未使用的输入引脚接地**」—— 8-bit 格式下 `D[9:8]` 需接地 |

引脚依据：`BBpro4_VU13P_FPGA_Display_FMC1DQ16_Pin_Assignment_20260915_v0.xlsx`
sheet「新版J4」。

### 原来的 case 现在怎么配

case 应该是"不同的流程"，而不是"同一个流程换个数字"。后者是宏：

| 原 case | 现在 |
|---|---|
| `snapshot-x2` | `SNAPSHOT` + `DCI_DEMO_SHOTS=2` + `BUF_COUNT=2` |
| `continuous-1buf` | `CONTINUOUS` + `BUF_COUNT=1` + `FILL_MODE=OVERWRITE` |
| `stress` | `CONTINUOUS` + `DCI_DEMO_FRAMES=1000` |
| `config-only` | `SNAPSHOT` + `DCI_DEMO_NO_CAPTURE=1` |
| `receive` | **不需要了** —— 两个采集 case 每帧都打地址和 JLink 命令 |

**删掉且无替代**（调用了宏到不了的驱动 API）：`signal-dump`、`probe-signal`、
`reconfig`、`suspend`、`raw10`。覆盖损失逐条记在
[TESTMATRIX.md §1](TESTMATRIX.md#一case-表)。

### JLink dump：用 `snapshot`

**快照帧在交付那一刻就是静态的** —— `CAPTURE` 在帧末**硬件自清**（spec §2.5.1），
没有任何东西还在写那块 buffer。所以不需要专门的 case，也不需要在 ISR 里抢时机：

```
=========== CAPTURE COMPLETE ===========
  seq 1, buffer 0, 153600 B
  buffer 0: 0x00810040 .. 0x00835840  (153600 B)
  JLink : savebin frame.bin, 0x00810040, 0x25800
  capture is stopped; the data is static and will not change
  set a breakpoint here, then dump with the command above
========================================
```

在这里下断点，然后 JLink 执行那条 `savebin`。要点：

- **校验失败时照样打地址** —— 没通过检查的那一帧最值得逐字节看；
- 帧长与配置不符只打 `NOTE` 不算失败，因为长短帧正是 dump 要解释的东西；
- `dci_demo.c` 为这个 case **跳过 teardown**：`Abort()` 会按约定作废所有已交付的帧，
  正是这里唯一不能做的事；
- 一帧都没收到时也打池的地址范围，因为"池在哪、多大"在排查"什么都没来"时同样是信息。

**连续模式下要 dump 某一帧**：设 `DCI_DEMO_CAPTURE_FRAME=N`。
帧计数走**始终使能**的帧中断，交付回调对第 N 帧**单纯不 release**，
并就地在 ISR 里清 `CR.capture`（`dci_demo_irq.c`）——
持有 lock 本来就是驱动自己的背压机制，不必和 DMA 抢时机。
随后用 `DCI_Capture_Stop()`（不动 `buf_lock_mask`）收尾。

> **`OVERWRITE` + 单 buffer 留不住帧**：环的下一个写入目标就是被留住的那块，
> 而 OVERWRITE 不会为它停。这个组合配 `CAPTURE_FRAME≠0` 会**编译期报错**。

## signal 表

13 行，从最容易跑通排到最刁钻。按顺序往下走，**第 N 行失败而 0~N-1 通过，
病因就锁定在第 N 行新增的那一项**。

摄像头侧 16 个 preset 映射到这里 12 行 —— **配置完全相同的不重复列出**。
crop / select / 降帧不是"行"，而是下面的 overlay，可以叠加到任何不冲突的行上。

RAM 列是池的需求量（含哨兵），对照 960 KB 池：

> **第 0 行的尺寸单位和别的行不一样。** 表里 `width` 对除 `NONE` 以外的格式是**像素**，
> 对 `NONE` 是**总线字节** —— 空配置不声明格式，就没有"像素"可数。所以第 0 行代码里写
> `width = 320` 而它的激励 `safe-160x120` 是 160 像素宽，**两者一致**：
> 160 个 RGB565 像素 = 320 总线字节，都是 320 pclk/行、38 400 B/帧。
> 对照两侧表格时**认 pclk/行，那是唯一同单位的量**。

| # | signal | 格式 | 尺寸 | 新增了什么 | 摄像头激励 | 帧 | 1 buf | 2 buf |
|---|---|---|---|---|---|---|---|---|
| 0 | `null-min` | **无** | 320 B/行 × 120 | **空配置**：`DCI_StructInit` 默认值，别无其他 | `safe-160x120`（160×120 px） | 38 KB | ✅ | ✅ |
| 1 | `qqvga-rgb565` | RGB565 | 160×120 | 最小正常帧，1 块 | `safe-160x120` | 38 KB | ✅ | ✅ |
| 2 | `qvga-rgb565` | RGB565 | 320×240 | 目标分辨率，**下面每一行的参照点** | `qvga-rgb565` 等 4 个 | 150 KB | ✅ | ✅ |
| 3 | `qvga-pck-rising` | RGB565 | 320×240 | `pckpol=1`：上升沿采样 | `qvga-pclk-rise` | 150 KB | ✅ | ✅ |
| 4 | `qvga-href-high` | RGB565 | 320×240 | `hspol=1`：HSYNC **消隐期为高** | `qvga-href-neg` | 150 KB | ✅ | ✅ |
| 5 | `qvga-vsync-high` | RGB565 | 320×240 | `vspol=1`：VSYNC **消隐期为高** | `qvga-vsync-neg` | 150 KB | ✅ | ✅ |
| 6 | `qvga-all-inverted` | RGB565 | 320×240 | 三个极性同时反转 → **位间耦合** | `qvga-all-inverted` | 150 KB | ✅ | ✅ |
| 7 | `qvga-raw8` | **RAW8** | 320×240 | 1 字节/像素。**RAW 对照组**，byte select 唯一可用格式 | `qvga-raw8` | 75 KB | ✅ | ✅ |
| 8 | `qvga-raw10` | **RAW10** | 320×240 | `EDM=10BIT`：唯一 pclk/像素 ≠ 字节/像素的行 | `qvga-raw10` | 150 KB | ✅ | ✅ |
| 9 | `qvga-embedded` | **YUV422** | 320×240 | `ESS` 内嵌同步码，不用同步引脚 | `ov5640-sync-yuv422` | 150 KB | ✅ | ✅ |
| 10 | `qvga-jpeg` | JPEG | 320×240 | 变长：buffer 是容量上限，帧在 `FF D9` 结束 | `qvga-jpeg` | 64 KB 上限 | ✅ | ✅ |
| 11 | `vga-rgb565` | RGB565 | 640×480 | 摄像头能出的最大尺寸，**3 块** | `vga-rgb565` | 600 KB | ✅ | ❌ |
| 12 | `466-raw8` | **RAW8** | 466×466 | 行长非 4 倍数 → **退化为 word 对齐分块**（全表唯一，且加 crop 会失效，见下） | — | 212 KB | ✅ | ✅ |

第 11 行只能单缓冲。配双缓冲会明确报 `DEMO_FAIL_POOL` 并打出差多少字节 ——
不会静默截断，因为短 buffer 的表现和采集故障一模一样，会白查半天。
这个尺寸本来就该配单 buffer + `OVERWRITE`（DESIGN.md §6.8）。

XGA（1024×768 RGB565，1572 KB）连单缓冲都装不下，所以**表里没有** ——
一行必然失败的 signal 只是噪音。

> **配 3~6 行之前先读这条，两处反相都会把答案弄反。**
> ① `hspol`/`vspol` 里的"有效"指**消隐期的电平**，不是有效数据期的。
> `hspol=0`（"HSYNC active low"）= 消隐期为低、**数据在高电平期被采**
> （TESTPLAN.md 3.1 的预期结果："只有 HSYNC 与 VSYNC 都处于**非有效期**的数据被采集"）。
> 所以摄像头的 `qvga-href-neg`（HREF 反相 → 消隐期为高）对应本机 `hspol=1` ——
> 行名里的 "high" 是寄存器自己的措辞，不是"高电平有效"。
> ② `pckpol` 是**本机采样**的沿，而摄像头 `COM10[4]` 是 sensor **更新**数据的沿，
> 两者相反（camera 侧 `ov2640_preset.h` 明写了这层关系）。下降沿更新的数据在上升沿附近才稳，
> 所以电气上正确的配对是**相反的沿**。表里目前配的是**同沿**，未经上板验证 ——
> 详见 `dci_demo_signal.c` 第 3~6 行上方的注释，第一次跑行 3 时两个值都试，
> 以数据为准再回来改。

加一行 signal 就多一种配置，不用改任何 case。

> 完整的逐行测试用意、几何/分块/池占用完整数据、以及「想验什么→用哪些行」
> 的反查表，见 **[TESTMATRIX.md](TESTMATRIX.md)**。本节只给选型需要的摘要。

### 第 0 行：空配置

`DCI_DEMO_SIG_NULL_MIN` 没有格式、没有极性、没有任何特性 ——
`DCI_InitTypeDef` 每个字段都是复位值，也就是 `DCI_StructInit()` 给的那套。
**先测基本的数据接收**，就从这里开始。

它存在的理由是：**「什么都没来」和「来了但不对」是两种病、一个症状**，
而其他每一行都可能同时产生这两者。这一行没有任何配置可以配错 ——
它失败了，病因就在时钟、引脚、DMA 或外设本身，再怎么改表都没用。
它是让其他行变得**可解释**的那一行。

它与第 1 行刻意相同，而且**不只是几何相同**：两行走完
`dci_demo_cfg_to_init()` 后得到的 `DCI_InitTypeDef` **逐字节相同**
（主机侧 `memcmp` 已验证 = 0）。原因是 `NONE` 在 width=320 下是 {1 字节, 1 pclk}、
`RGB565` 在 width=160 下是 {2, 2}，都得到 320 pclk/行 × 120 行 = 38400 B；
两行唯一不同的字段 `fmt` 对这两个取值**根本到不了寄存器**
（它只决定 `edm`——两行都是 8BIT——和 `jpeg`——两行都是 false）。

所以按「**重复配置不需要列出**」的规则，**这两行是同一个配置**，第二行是刻意留的。
差别不在"硬件被告知了什么"，而在"**demo 声明了什么**"：第 1 行声明了格式、像素数、
每像素字节数；第 0 行什么都不声明。**同样的字节，经由一个对它毫无主张的配置进到内存。**
第 0 行过、第 1 行不过 → 病因在 demo 自己的算术，不在外设 —— 单独任何一行都证明不了这件事。

### 摄像头侧哪些行被合并了

DCI 看不到的差别就不该占一行。合并关系（`dci_demo_signal.c` 里写明了每一条）：

| 摄像头 preset | 归到 | 理由 |
|---|---|---|
| `qvga-yuv422` | 行 2 | 2 字节/像素、2 pclk/像素，几何完全一致，接收侧无法区分 |
| `qvga-colorbar` | 行 2 | 图案在 sensor 内部生成，接收配置逐字节相同 |
| `qvga-pclk-gated` | 行 2 | 「消隐期无 PCLK」在接收侧**没有对应配置位**，DCI 只数给它的边沿。值得**作为激励**去跑行 2 |
| `ov5640-hwsync-yuv422` / `ov5640-rgb565` | **都归到行 2** | 两者都是 320×240 硬件同步，已覆盖；sensor 型号不是接收侧变量。`ov5640-rgb565` 更是与行 2 逐字段相同 |
| `safe-160x120` | 行 0 **与**行 1 | 唯一一个激励对两行：同样的字节，一次经空配置、一次经 RGB565 声明（见上节） |

合并本身就是结论：**它明确说出这个外设对哪些激励差异是盲的。**

> **什么是"退化分块"**：GDMA 的传输单位是 **word（4 字节）**，一个描述符最多 65 535 word，
> 所以驱动把一帧切成若干块。行长是 4 的倍数时，一行 = 整数个 word，块**按整行**切、
> 边界必落在行首；行长不是 4 的倍数时（466 B = 116.5 word），行不再是可用的切割单位，
> 驱动**改按 word 切整帧**，偏移公式从 `first * line_bytes` 变成 `first * 4`，
> `lines_per_chunk` 置 0。这就是「退化」——
> 驱动自己的措辞（`rtl_dci.c` dump 打 `word-aligned(degenerate)`）。
> 它是一条**独立的几何计算路径**，走错了不报错、只是把数据放到错地址。
> 详细对照表见 [TESTMATRIX.md](TESTMATRIX.md) §3。
>
> **第 12 行是这四行里唯一"不可替代"的**，而且它有一个静默失效方式。
> 它是全表唯一到得了 `dci_calc_geometry()` 退化分块分支（`line_aligned = 0`）的行 ——
> 主机侧 13 行 × 32 个 overlay 组合全扫实测，只有它、只在 4 个组合里到达。
> 其余各行的行长（320/640/932/1280/1600 × 1~2 字节）都是 4 的倍数。
> ⚠️ **给它加 crop 会销毁这一点**：`DCI_ConfigCrop()` 把宽度向下对齐到 4 pclk，
> `CROP=FULL` 把 466 变 464，行长重新对齐、这行不再走那条分支，**且不报任何错**。
> 跑它用 overlay 全关，或只加 `LSM` / `BSM=1`。详见 [TESTMATRIX.md](TESTMATRIX.md) §3。

### 第 12 行为什么没有摄像头激励

OV2640 出不了这些几何（RAW 被它的窗口表锁在 320×240，而没有 sensor 输出奇数宽度）。
保留它们是因为**它们触发的接收侧行为必须有人覆盖**：word 对齐退化分块、
2 块与 4 块路径、双缓冲上限、池上限。

对着实际摄像头它们会因帧长不符而失败，**这是预期行为**，表里的 note 写明了。
它们的价值在配置期（池算术、块数推导）和对着码型发生器时。

---

## overlay

接收侧特性，叠加在选中的那行 signal 之上。**全部默认关**，所以默认构建就是
「来什么收什么」—— 一次只加一项，理由和 signal 行的排序理由相同。

| 宏 | 取值 | 说明 |
|---|---|---|
| `DCI_DEMO_OV_CROP` | `OFF` / `CENTER` / `ORIGIN` / `FULL` | 裁剪窗口，**按行尺寸的比例**而非绝对像素 |
| `DCI_DEMO_OV_BSM` | 0 / 1 / 2 / 3 | byte select：全部 / 二取一 / 四取一 / 四取二 |
| `DCI_DEMO_OV_OEBS` | 0 / 1 | byte select 取奇相位 |
| `DCI_DEMO_OV_LSM` | 0 / 1 | line select：隔行抽取（抽整行，与格式无关） |
| `DCI_DEMO_OV_OELS` | 0 / 1 | line select 取奇行 |
| `DCI_DEMO_OV_FCRC` | 0 / 1 / 2 | 硬件降帧：全部 / 二取一 / 四取一。**仅连续模式** |

crop 用比例而非绝对像素：表跨 160×120 到 800×600，一个绝对窗口在一端越界、
在另一端等于全帧，比例则在每一行都有效。

| crop 档 | 窗口 | 测什么 |
|---|---|---|
| `CENTER` | 双轴减半、居中 | 常规用法；x/y 或 w/h 写反会表现为窗口偏移而不是失败 |
| `ORIGIN` | 双轴 1/5、贴 (0,0) | **x0 = y0 = 0 边界**，N-1 编码（`capcnt = w-1`）的差一错误只在这里看得见 |
| `FULL` | 全帧，但 CROP 使能 | 看着没用，其实最有用：结果必须和 `CROP_OFF` **逐字节相同**。不同就说明**使能 CROP 本身**是坏的，这和窗口算错是两种病，否则无法区分 |

### overlay 的拒绝规则

分两类。第一类驱动本来也会拒，在这里拦是为了**让报错指名该改哪个宏**，
而不是从 `Capture_Init` 收到一个 `MODE_CONFLICT` 却不知道四条规则里踩了哪条：

| 组合 | 结果 |
|---|---|
| crop + JPEG | 拒绝（spec §2.7 互斥） |
| byte select + RAW10 | 拒绝（byte select 要求 `EDM=8BIT`） |
| byte select 组长除不尽行长 | 拒绝，并报出行长和组长 |
| 内嵌同步 + RAW10 | 拒绝（spec §2.4 内嵌同步只支持 8-bit） |

**第二类才是危险的：驱动会接受、几何算得出、采集不报错、图像静默地错。**
表是唯一的防线：

| 组合 | 为什么必须拦 |
|---|---|
| byte select + 2 字节/像素 | 抽的是**字节**。1 字节/像素时等于"抽像素"；2 字节/像素时是从每个像素里抽掉一个字节，留下一 buffer 半个像素。**DCI 没有"像素"这个概念，硬件不可能拦** |
| byte select + JPEG | 熵编码流被抽掉一个字节不是"抽稀"，是**从那个字节起整条流报废** |
| 降帧 + JPEG | JPEG 按内容结束，中途丢一帧和截断一帧无法区分 |

> **byte select 的组长必须整除行长。** 驱动在 `dci_calc_geometry()` 里对
> `line_bytes % bsm_den != 0` 返回 `INVALID_PARAM` —— 行尾留一个不完整的组会让
> **后面每一行都错位**，所以它拒绝而不是猜。
> 可用行只有三行：`null-min` 与 `qvga-raw8`（320 B/行，四档都行），
> 以及 `466-raw8` **只能用第 1 档** —— 466 是偶数但不是 4 的倍数。

`OEBS`/`OELS` 在对应的 select 关闭时没有意义，会在**编译期** `#error`（设错宏的信号）。
`FCRC` 只在连续模式存在（spec §2.5），配 `snapshot` 同样编译期报错 ——
一个被静默忽略的配置比一个编译错误糟得多。

### 为什么 10-bit 值得单独一行

> ⚠️ 原来的 `raw10` case 已删除，**下面这套逐样本校验目前没有 case 在跑**。
> 校验器还在（`dci_demo_buf_raw10_bad_samples()`），在 `snapshot` 里对 RAW10 行
> 调一次就能恢复。

**它是全表唯一具备「与画面内容无关的逐样本不变式」的格式。**

其他格式对着实景只能问「数据到了没」（非全 poison、非全同值）；
10-bit 下硬件把每个样本放在一个 16-bit 半字的低 10 位，**bit15:10 必须为 0**
（DESIGN.md §2 数据摆放）。这条规则不依赖镜头对着什么，所以可以**逐样本穷举**。

效果是把「图像看着不对」变成「第 4097 个样本是 0x8000」，
而且**坏样本的比例直接指出病因**：

| 坏样本分布 | 结论 |
|---|---|
| 100% | `EDM=10BIT` 根本没生效 —— 实际在跑 8-bit，两个 8-bit 像素被当成一个样本 |
| 50%（隔一个） | 半字相位错一个字节 —— 某个分块边界落在样本中间 |
| 尾部连续一段 | 帧比声明的长，多出来的部分按 8-bit 写入 |

case 还会报样本的 min/max/mean：全平坐实「没搜到数据」，
而「全部 ≤ 255」提示 D[9:8] 可能没接或 sensor 实际输出 8-bit（暗场景也会如此，
不是定论）。

**两条互斥拒绝也在这个 case 里断言**（不需摄像头，在采集之前跑）：
`BSM` 要求 `EDM=8BIT`，内嵌同步只支持 8-bit。这两条是最容易默默烂掉的那类校验：
若将来有人放宽了 `dci_validate_init()`，错误配置会被默默接受，
症状是图像错位，而人会去查接线。

`raw10` case 采集前会交叉校验 `line_bytes == 2 × pclk_per_line` ——
帧字节数相同而几何不同的情况不会被帧长校验抓到，只能这样抓。

> 原先还有第 15 行 `vga-raw10`（字节数与 `vga-rgb565` 相同而 pclk/行只有一半，
> 两者分块数都是 3 → 证明**分块看字节而不是 pclk**）。该行已按要求删除，
> 这个对照关系现在没有行覆盖，10-bit 也只剩第 8 行一行。

> 寄存器表里 `edm` 是 2 bit，看着像有 12/14-bit 档。物理上只有 D[9:0] 十条线，
> 12/14-bit 是 STM32 残留，枚举只暴露两档（DESIGN.md §2）。

---

## 中断

五个中断源，三个常开、两个可选：

| 源 | 默认 | 说明 |
|---|---|---|
| `FRAME` | **常开** | 驱动关帧要用，不可协商。逐帧报告和「留第 N 帧」都走它 |
| `OVR` | **常开** | 无条件打 log |
| `ERR` | **常开** | 无条件打 log |
| `VSYNC` | **关** | `DCI_DEMO_IRQ_VSYNC=1` 开启，累计计数 |
| `LINE` | **关** | `DCI_DEMO_IRQ_LINE=1` 开启，逐帧 + 累计计数 |

**"关"是指 IER 里被屏蔽，不是收了再丢。** 机制是驱动自己的、未经修改的：
`dci_int_config()` 只在 `DCI_DMA_config` 里对应的回调指针非 NULL 时，
才把 `IT_VSYNC` / `IT_LINE` 加进 IER。所以**让指针保持 NULL 就是寄存器级的关闭** ——
宏控制的是指针，不是之后再补一次屏蔽，也不存在"中断已开但没人处理"的窗口。

> **顺序上有一个坑，而且它是静默失效的**：`DCI_Capture_Start()` 会在使能外设前调用
> `dci_int_config()`，那是一次**赋值**。之前用 `DCI_INTConfig()` 设的 IER 位会被抹掉，
> 之后设的则和第一帧抢跑。**给回调、让驱动自己推导 IER**，是两头都正确的唯一做法。

`DCI_DEMO_IRQ_LOG=1` 时，**每个帧中断打一行**：本帧行数 + 累计 sync 数。

```
[IRQ] frame 3  seq 3  buf 1  153600 B  lines 240  vsync 3 (total)
```

两个数放在一起就说明了**接收侧和摄像头是否还对几何有共识**。
行数为 0 表示 `IRQ_LINE` 没开、sync 为 0 表示 `IRQ_VSYNC` 没开 ——
两列都照打，因为缺一列的 log 比带个 0 的 log 更难读。
最终报告里会明确说明哪个源被屏蔽了：`vsync=0` 有歧义，「该中断没开」没有。

**ISR 里除错误路径外一律不打印。** LINE 在 466 行、60 fps 下约
**每秒 28000 次中断**；在那里打印不是让报告变慢，而是**直接造成它本要报告的那个溢出**。
所以计数在 ISR 里累加，统一在帧中断里打一行。

`OVR` 与 `ERR` 无论 `IRQ_LOG` 怎么设都打 log —— 它们按定义就很少发生，淹不了日志，
而**静默的 overrun 是这个外设最贵的失败模式**：采集照跑、帧数照涨，
唯一的痕迹在一个没人看的统计计数器里。log 里带 `RIS` 快照，
因为那是唯一能看到"故障那一刻哪些位挂起"的东西 —— 等线程去看时驱动已经清掉了。

---

## 编译期配置

全在 `dci_demo_config.h`，都带 `#ifndef` 可被 `-D` 覆盖：

| 宏 | 默认 | 说明 |
|---|---|---|
| `DCI_DEMO_CASE` | `REG_TEST` | **测哪一项**。见 `dci_demo_id.h` |
| `DCI_DEMO_SIGNAL` | `QQVGA_RGB565` | **引脚上是什么**（160×120，默认 40 KB 池装得下）。`REG_TEST` 忽略 |
| `DCI_DEMO_OV_*` | 全 0 | overlay，见上一节 |
| `DCI_DEMO_BUF_COUNT` | **1** | 1 或 2。**不再被任何 case 覆盖**；40 KB 池下 160×120 只够单缓冲 |
| `DCI_DEMO_SHOTS` | 1 | **`snapshot` 拍几张**。2 = 双缓冲轮转检查，须配 `BUF_COUNT=2` |
| `DCI_DEMO_NO_CAPTURE` | 0 | 1 = **只配置不采集**，不需摄像头（原 `config-only`） |
| `DCI_DEMO_FILL_MODE` | `GATE` | **buffer 满了怎么办**：`GATE` 不覆写 / `OVERWRITE` 覆写 |
| `DCI_DEMO_IRQ_VSYNC` | 0 | 寄存器级开启 `IT_VSYNC` |
| `DCI_DEMO_IRQ_LINE` | 0 | 寄存器级开启 `IT_LINE`（注意带宽，见上） |
| `DCI_DEMO_IRQ_LOG` | 0 | 每帧打一行：本帧行数 + 累计 sync 数 |
| `DCI_DEMO_CAPTURE_FRAME` | **0** | **`continuous` 留第几帧**供 dump，0 = 不留。`snapshot` 不需要（帧交付即静态） |
| `DCI_DEMO_RAM_BUDGET_KB` | **40** | 总预算，只用于一致性检查。与 `POOL_KB` 一起调 |
| `DCI_DEMO_POOL_KB` | **40** | framebuffer 池，放在 `.dci_pool` 段（见 RAM 一节） |
| `DCI_DEMO_JPEG_CAP_KB` | 64 | JPEG 容量上限 |
| `DCI_DEMO_FRAMES` | 8 | **连续模式采多少帧**。1000 = 原 `stress` |
| `DCI_DEMO_TIMEOUT_MS` | 3000 | 单帧超时 |
| `DCI_DEMO_DUMP_LEVEL` | 1 | 0 摘要 / 1 抽样 / 2 全量 hex。连续模式强制 0 |
| `DCI_DEMO_SECTION` | `.dci_pool` 段属性 | 池的放置。**需要 scatter 里有对应域**，见 RAM 一节 |
| `DEMO_LOG` | `DBG_DIRECT` | 日志出口 |
| `DCI_DEBUG_REG_DUMP` | 0 | **建议置 1** —— 否则失败时没有寄存器 dump |

配置里的 `#error` 会挡掉：case/signal/crop/BSM/FCRC/`SHOTS` 越界、buffer 数非 1/2、
填充方式越界、池超预算、dump level 越界、`OEBS`/`OELS` 没有对应的 select、
`FCRC` 配到 `snapshot`，以及三个新增的组合检查：

- `SHOTS=2` 配 `BUF_COUNT=1` —— 两张拍进同一个 buffer 看着像通过，其实什么都没测
- `CAPTURE_FRAME≠0` 配 `snapshot` —— 快照帧交付时已经静态，这个宏没有意义
- `CAPTURE_FRAME≠0` 配 `BUF_COUNT=1` + `OVERWRITE` —— 留不住，见上

### 填充方式

| 档 | 行为 | 代价 |
|---|---|---|
| `GATE` | 清 `CAPTURE` 等消费者，**绝不撕裂** | 负载高时丢帧 |
| `OVERWRITE` | 满速继续写，可能交出**半新半旧**的 buffer | 不丢帧但可能撕裂 |

默认 `GATE`，理由是**撕裂帧比丢帧难认得多**。但要留意：
**单 buffer + `GATE` 会把连续采集变成"每帧之后停一下"**，取景器要的不是这个 ——
所以这一项必须显式选，不能默认了算。

> **crop 配置要按未裁剪尺寸分配 buffer。** `DCI_Capture_Init()` 不接受 crop 窗口
> （窗口由之后的 `DCI_ConfigCrop()` 下发），所以 Init 校验的是**全帧**大小。
> 按裁剪后尺寸分配会被 `BUF_TOO_SMALL` 拒掉。`dci_demo_cfg_t` 里
> `init_bytes` 与 `frame_bytes` 分开就是为此，`signal-dump` 会把两个尺寸都列出来。

---

## 自动校验

真 sensor 对着实景没有 golden 数据，所以断言全部是**与画面内容无关**的结构性检查：

| 检查 | 能抓到 |
|---|---|
| 几何交叉校验（demo 推导 vs 驱动推导） | 两份公式不一致 —— **最该先抓的** |
| `bytes == frame_bytes` | 几何声明错、分块算错、crop 静默截断 |
| `!truncated` | 帧偏长/偏短、JPEG 越界 |
| 哨兵未变 | 描述符 `DAR`/`BLOCK_TS` 越界写 |
| 非全 poison、非全同值 | 完全没搬到数据、总线卡死 |
| `seq` 连号 | 交付了但没被取走 |
| `index` 轮转 | 双缓冲切换失效 |
| JPEG `FF D8` … `FF D9` + 0~3 补零 | JPEG 通路与补零行为 |
| `DCI_GetStats()` 除 `frames_done` 外全 0 | OVR/ERR/resync/buf_overflow 任一非 0 |

`continuous` 的留帧路径（`CAPTURE_FRAME≠0`）只做哨兵检查 ——
越界写已经破坏了别的东西，那值得在信任 dump 之前知道。其余一律只报不判：
**判数据是用户的活**。帧长和配置不符也只打 NOTE 不算失败，
因为长短帧正是 dump 要解释的东西，拒绝打印地址等于毁掉唯一的证据。
`snapshot` 的自动校验照跑，但**地址无论通过与否都打**。

失败时自动 `DCI_RegDump()`（需 `DCI_DEBUG_REG_DUMP=1`）—— 出错那一刻的寄存器
状态才有诊断价值。

## dump 分级

`dci_demo_buf_dump(f, p, level)`：

| level | 内容 | 用在 |
|---|---|---|
| 0 | 摘要 + CRC32 | 连续模式（强制） |
| 1 | 头尾字节 + 每隔若干行的行首 | 快照。**行首规律断了就是分块/stride 错** |
| 2 | 全量 hex，带地址前缀 | 疑难排查，PC 侧可还原成图 |

连续模式强制 level 0：424 KB 全量 hex 约 7 s @1 Mbps，打印本身会造成它要
检测的溢出。**要看全帧数据请用 `snapshot` + JLink**，而不是把 hex 打到串口上。

---

## 主机侧验证

`rtl_common` 与 SoC 无关，demo 的软件路径也是，所以可以在 PC 上编译并部分执行
（stub 掉 SoC hook 与 DMA 平台层，寄存器块重定向到主机 RAM）。
**驱动 `rtl_dci.c` 和它的头文件一行未改** —— 主机侧用的是头文件的一份补丁副本。
实测结果：

- **3 个 case 各自单独编译，全部 0 warning**（`-Wall -Wextra`），与真实
  `rtl_dci.c` 完整链接通过
- **宏组合矩阵全绿**（15 组，全部 0 warning）：`reg-test`；`snapshot` 默认 /
  `NO_CAPTURE=1` / `SHOTS=2`+`BUF_COUNT=2` / `BUF_COUNT=1` / `DUMP_LEVEL=2`；
  `continuous` 默认 / `FRAMES=1000` / `BUF_COUNT=1`+`OVERWRITE` /
  `CAPTURE_FRAME` ∈ {1, 1000} / 三个中断宏全开；两者各配 `DCI_DEBUG_REG_DUMP=1`
- **编译矩阵全绿**：2 个采集 case × 13 行 signal × 4 档 crop = **104 组，0 warning**
- **6 条非法组合逐条确认被编译期 `#error` 挡住**：`SHOTS=2`+`BUF_COUNT=1`、
  `CAPTURE_FRAME≠0` 配 `snapshot`、`CAPTURE_FRAME≠0`+`BUF_COUNT=1`+`OVERWRITE`、
  `FCRC≠0` 配 `snapshot`、`DCI_DEMO_CASE=3`（越界）、`SHOTS=3`
- **`NO_CAPTURE` 路径在主机上实跑通过**：banner 正确标 `[no camera needed]`，
  打印完整配置、池地址与两条 `savebin` 命令，`PASS`
- 三个 case 的 banner 各自实跑确认：`snapshot` 打 `shots`/`capture` 两行，
  `continuous` 打 `frames`/`hold` 两行，`reg-test` 两者都不打
- **几何交叉校验对着真驱动跑通**：对每一行走
  `dci_demo_cfg_resolve` → `dci_demo_cfg_to_init` → `DCI_Capture_Init` →
  `DCI_ConfigCrop` → `DCI_CropCmd` → `DCI_Capture_GetFrameInfo`，
  比对 `info.frame_bytes` 与表推导值：**13 行 0 mismatch**，
  且在 crop 三档、BSM 四档、`LSM`、`LSM+OELS`、`FCRC`、以及
  crop×BSM、crop×LSM 等叠加下**同样 0 mismatch**
- **摄像头侧对照**：12 行带激励的 signal 与 `preset-table.md` 的宽高/帧字节/pclk
  逐项比对，**0 disagree**
- 分块数与每行声明的用意一致：`466-raw8` → word 对齐退化分块、
  `vga-rgb565` → 3 块、`qvga-jpeg` → 容量上限单块
- 拒绝规则逐条验证会触发且报错指名宏：BSM 在每个 2 字节格式上被拦、
  BSM+RAW10、BSM+JPEG、BSM 组长除不尽行长、crop+JPEG、FCRC+JPEG
- 越界 signal 在编译期被 `#error` 挡掉
- **10-bit 校验器做过注入测试**（手构帧）：干净帧 → 0 坏样本；全部高位置 1
  → 报 100% 并给出「EDM 未生效」；隔一个置 1 → 报 50% 并给出「半字相位错一字节」；
  全平帧与全部 ≤ 255 也各自报对。⚠️ **该校验器目前没有 case 在调用**

**主机侧跑出来的 bug**（均已修）：

1. `dci_demo_buf_alloc()` 用 `uint8_t` 循环变量数到 65536 —— 无限循环。上板会在
   第一个 case 就挂死，而且没有任何输出。
2. crop 配置按裁剪后尺寸分配 buffer，被 `Capture_Init` 以 `BUF_TOO_SMALL` 拒绝。
3. `dci_demo_overlay_apply()` 在 crop 关闭时 `s` 未使用 —— `-Wall` 报警。
4. BSM + JPEG 的报错信息说"0 字节/像素的格式"，对表项而言是真的，作为诊断毫无意义。
5. 第 0 行的 RAM 列写成 19 KB，而推导值是 38400 B。
6. **BSM 组长除不尽行长时表会四舍五入，而驱动直接拒绝** ——
   `466-raw8` 配四取一/四取二时 demo 算 116 B/行、驱动返回 `INVALID_PARAM`。
   交叉校验抓到的正是这个：现在 resolver 会拒绝并报出行长与组长。

---

## 上板顺序

编译期选择意味着每一步都要重编重烧，所以这张表就是操作清单。
**首个失败即停** —— 后面的步骤都建立在前面的结论上，跳着测等于放弃定位能力。

| 步 | `DCI_DEMO_CASE` | `DCI_DEMO_SIGNAL` | 其他宏 | 在证明什么 |
|---|---|---|---|---|
| 1 | `REG_TEST` | — | — | 寄存器可达：时钟开了、基址对、位正常 |
| 2 | `SNAPSHOT` | `NULL_MIN` | `NO_CAPTURE=1` | 配置能被接受、池够用、几何与驱动一致（**不需摄像头**） |
| 3 | **`SNAPSHOT`** | **`NULL_MIN`** | 默认（`BUF_COUNT=1`） | **数据真的进了内存** —— 空配置，眼睛看字节 |
| 4 | `SNAPSHOT` | **`QQVGA_RGB565`** | 默认 | **160×120 全自动校验通过** —— 40 KB 池的目标 |
| 5 | `SNAPSHOT` | `QQVGA_RGB565` | `SHOTS=2` + `BUF_COUNT=2` + **池 80 KB** | 双缓冲轮转 |
| 6 | `CONTINUOUS` | `QQVGA_RGB565` | `BUF_COUNT=2` + **池 80 KB** | 闭环、会合、门控 |
| 7 | `CONTINUOUS` | `QQVGA_RGB565` | `BUF_COUNT=1` + `FILL_MODE=OVERWRITE` | 单缓冲覆写（断言 `suspend_count=0`） |
| 8 | `CONTINUOUS` | `QQVGA_RGB565` | `FRAMES=1000` | 长跑，盯统计和哨兵 |

之后再按需逐项加 overlay、逐行换极性/格式 —— 一次只动一个变量。

**第 2 步不需要摄像头**，可以在拿到 sensor 之前就把整张 signal 表和各档 overlay
逐个汆一遍：它跑完整的 `Capture_Init` + crop + 几何交叉校验，只是不启采集。

**第 3 步用第 0 行空配置**：这一步要回答的是"数据能不能进内存"，
而空配置是唯一一个「失败了就一定不是配置问题」的选择。地址会被打出来，
在那里下断点、用打印的 `savebin` 命令 dump 即可。

> **多块组帧暂时跑不了。** 原第 7 步用 `VGA_RGB565`（600 KB，3 块）验证多块路径，
> 而 DTCM 总共 128 KB 放不下。要覆盖它，池得挪到 DSP_SHM 之类的大区域
> （见上面「想跑更大的行」）。当前这套 40 KB 配置只覆盖**单块**路径。

### ⚠️ 帧一直不来时：先看 `RIS`，**不要**看 `SR` 的电平

`RIS = 0` 而 `IER` 里 LINE 已开，是最强的一条证据：**`RIS` 的置位与 `IER` 无关**，
所以 `RIS=0` 意味着**一个行边界都没被识别过**。再配合 `SR.FNE=0`
（一个字都没进过 DCI FIFO），故障就锁定在 **DCI 的像素输入侧，在 DMA 之前** ——
此时 DMA 寄存器全对是**不相干**，不是"令人安心"。`CURR_DAR == DAR` 是结果而非原因。

**`SR` 的电平不能用来判故障**，这个坑已经踩过一次：

- `SR` 是**组合逻辑**，读的是 `引脚 XOR 极性位`。翻 `hspol/vspol`
  会让读数跟着翻，而引脚一动没动 —— 所以"`SR` 变了"不能证明线上有变化。
  实测两次：`hspol=0/vspol=0` 读到 `SR=0x03`，`hspol=1/vspol=1` 读到 `SR=0x00`，
  两次对应的**原始引脚电平是同一个**。
- 它只是**某一瞬间的一次采样**。HSYNC 在一行里有效约 85%（320/376 pclk），
  VSYNC 在一帧里有效更久，所以"读到有效"对**健康信号**和**卡死的引脚**
  都是最可能的采样结果，区分不了。

能区分的只有**在长于一帧的窗口里看引脚是否翻转**，这就是 `dci_demo_sync_probe()`：
`RIS` 仍为 0 时它会在阻塞等待**之前**自动跑，四种结论各自对应一个原因 ——
两路都翻转但 `RIS=0` → **PIXCLK 没到外设**；从不翻转 → 引脚卡死/sensor 没出流；
只有 HSYNC 翻转 → VSYNC 没接或窗口太短；`RIS` 变非零 → 之前那次读得太早。

主机侧自测：`test/probe_selftest.c`（注释里有重新生成 + 编译的命令，
函数正文从 `dci_demo_irq.c` 原样抽取，不会与已发布代码脱节）。

> **少了一步预检。** 原来第 4 步是 `PROBE_SIGNAL`：不启 DMA、只数中断，
> 直接测出 sensor **真实**输出多少行。该 case 已删除，所以
> "配置声明 240 行、sensor 实际输出 480 行"这类集成期常见错误
> 现在只能以 `FRAME_LEN` 或 OVR 的形式暴露，需要反推。
> 开 `DCI_DEMO_IRQ_VSYNC=1` / `IRQ_LINE=1` + `IRQ_LOG=1` 能拿到逐帧行数与
> 累计 sync 数，是最接近的替代。
