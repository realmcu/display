# DCI Driver Design

Digital Camera Interface (DCI) 驱动设计文档。

状态：**设计阶段**，寄存器定义已完成，其余代码待实现。

---

## 1. 命名与背景

同一个 IP 有三个名字，来源不同：

| 名字 | 出处 |
|---|---|
| **DCI** | 硬件 spec 的正式名（Digital Camera Interface）。**驱动统一用这个**：`DCI_` 前缀、`hardware/dci/`、`rtl_dci*.{h,c}` |
| DCMI | RTL 寄存器表的 `IP name` 字段，寄存器命名为 `REG_DCMI_*` |
| DVP | 早期目录名，已废弃 |

这个 IP 是 **STM32 DCMI 的克隆**：寄存器集 CR / SR / RIS / IER / MIS / ICR / ESCR / ESUR / CWSTRT / CWSIZE / DR 与 DCMI 逐位一致。因此 ST 的 `stm32f4xx_hal_dcmi.c` 是可直接参考的实现范本（见 §16）。

官方测试用例整理见 [TESTPLAN.md](TESTPLAN.md)，裸机测试工程架构见 [TESTAPP.md](TESTAPP.md)，
demo 操作说明见 [test/README.md](test/README.md)，
**case 与 signal 速查表见 [test/TESTMATRIX.md](test/TESTMATRIX.md)**。

### 参考文档

| 文档 | 路径 |
|---|---|
| 寄存器表 | `D:\DocDev\DVP\DCI\BBPro4_DCMI_register_table_20260416v1 (3).xlsx`，sheet `DCMI` |
| 硬件 spec | `D:\DocDev\DVP\DCI\BBpro4_DCI_requirement_spec_20260323 (1).docx` |
| ST 参考实现 | `realtek-app/applications/eBadge/component/FlashDB/demos/stm32f405rg/libraries/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dcmi.c` |

> spec 中两处数值作者自己标了「？」，不可当硬指标：FIFO 深度「8-word？」、速率「可达 54MB/s?」。

---

## 2. 硬件要点

Base address **`0x4064_3000`**，挂 **AHB**，11 个寄存器。

### 基址以 address_map.h 为准，不是寄存器表

**寄存器表写的 `0x4001_7000` 不能用。** 目标 SoC 是 RTL87X3K（BBPro4），其 `address_map.h`（文件头注明 `Reference: BBPro4_RLE1146_AddressMap_20260820v0.xlsx`）给出：

```
#define DCI_BASE    0x40643000
#define DCI_TOP     0x40643fff
```

DCI 位于 **display island** 内，邻居可佐证：

| 外设 | 基址 |
|---|---|
| JPEG decoder | `0x4064_0000` |
| MIPI | `0x4064_1000` |
| SPI JDI | `0x4064_2000` |
| **DCI** | **`0x4064_3000`** |
| PNG decoder | `0x4064_4000` |
| display AON / platform / PCC | `0x4064_5000` / `6000` / `7000` |
| IDU | `0x4065_0000` |

而 `0x4001_7000` 在这颗上落在低位外设块（`0x4001_xxxx` 是 ADC/UART/SPI/I2C/IR 那一片）—— 按寄存器表的值写下去等于戳错外设。寄存器表的 `0x4001_7000` 应是另一颗 SoC 的地址，**偏移与位域仍然有效，只有基址要换**。

> 教训：寄存器表给的是 IP 视角的偏移与位域，**基址是 SoC 集成决定的**，必须以 `address_map.h` 为唯一真源。

```
D[9:0]   10 条并行数据输入      ← 只有 10 条，见下方 EDM 说明
HSYNC    行同步输入，极性可配
VSYNC    帧同步输入，极性可配
PIXCLK   像素时钟输入，极性可配
```

DCI 全部 IO 都是输入，纯从设备。

### 寄存器映射

| Offset | 名称 | 属性 | 用途 |
|---|---|---|---|
| 0x00 | `CR` | R/W | 控制：capture / cm / crop / jpeg / ess / 极性 / fcrc / edm / enable / 行列抽取 |
| 0x04 | `SR` | R | 状态：`hsync` / `vsync` / `fne` |
| 0x08 | `RIS` | R | 原始中断状态 |
| 0x0C | `IER` | R/W | 中断使能 |
| 0x10 | `MIS` | R | 屏蔽后中断状态 |
| 0x14 | `ICR` | R/W1S | 中断清除，**写 1 有效，写 0 无效** |
| 0x18 | `ESCR` | R/W | 内嵌同步码 FSC/LSC/LEC/FEC |
| 0x1C | `ESUR` | R/W | 内嵌同步码取消掩码 |
| 0x20 | `CWSTRT` | R/W | 裁剪窗口起点 `vst` / `hoffcnt` |
| 0x24 | `CWSIZE` | R/W | 裁剪窗口大小 `vline` / `capcnt` |
| 0x28 | `DR` | R | 数据寄存器（FIFO 出口）**读取会弹出数据** |

中断位序（`RIS`/`IER`/`MIS`/`ICR` 一致）：`FRAME`[0] `OVR`[1] `ERR`[2] `VSYNC`[3] `LINE`[4]。5 个中断源共用一根全局 IRQ，`IT_DCI` 是它们的逻辑或。

### 硬约束

| 约束 | 出处 | 驱动动作 |
|---|---|---|
| **配置寄存器必须在 `CR.ENABLE=0` 时写** | 寄存器表 `enable` 字段注释 | 决定了运行时重配置流程，见 §12 |
| `CAPCNT` 必须是 4 的倍数，低 2 bit 被硬件强制清 0 | spec §2.6 | 向下对齐 + 告警 |
| 非 JPEG 最大 2048×2048 | spec §2.9 | 参数校验 |
| `EDM` 只有 bit0 有效：0=8-bit，1=10-bit | spec §2.1，只有 D[9:0] | 枚举只暴露两档。寄存器表里的 12/14-bit 是 STM32 残留，物理上不存在 |
| 内嵌码同步仅支持 8-bit，且数据不能含 0x00/0xFF | spec §2.4 | 与 `EDM=1` 互斥 |
| JPEG 模式不支持 crop 和内嵌码同步 | spec §2.7 | 互斥校验 |
| `PIXCLK` 周期必须 > 2.5 个 `HCLK` 周期 | spec §2.2 | 时钟配置检查，文档提示 |
| 仅 32-bit word、仅小端 | spec §2.9 | 无 endian 配置项 |
| FIFO 无溢出保护，溢出后 FIFO 复位并等下一帧 | spec §2.8 | 溢出不可挽救，整帧丢弃 |

### 数据打包

8-bit 模式（`EDM=0`），4 个 PIXCLK 组成一个 word：

| Byte addr | 31:24 | 23:16 | 15:8 | 7:0 |
|---|---|---|---|---|
| 0 | Dn+3 | Dn+2 | Dn+1 | Dn |

10-bit 模式（`EDM=1`），2 个 PIXCLK 组成一个 word，每个样本占 16 bit 的低 10 位，bit15:10 清 0：

| Byte addr | 31:26 | 25:16 | 15:10 | 9:0 |
|---|---|---|---|---|
| 0 | 0 | Dn+1[9:0] | 0 | Dn[9:0] |

`CAPCNT` 被强制为 4 的倍数正是为了保证行长可被 word 整除：8-bit 下 4 pclk = 4 字节 = 1 word；10-bit 下 4 pclk = 8 字节 = 2 word。

---

## 3. 文件布局与分层

照 IDU 的三层结构，SoC 相关项全部收进最内层：

```
hardware/dci/
├── DESIGN.md                       本文档
├── CMakeLists.txt                  CONFIG_REALTEK_DISPLAY_ENABLE_DCI
├── inc/
│   ├── CMakeLists.txt
│   └── rtl_dci.h                   公共 API：枚举 / InitTypeDef / DMA_config / 内联 cmd
└── src/
    ├── CMakeLists.txt
    └── device/
        ├── CMakeLists.txt
        ├── rtl_common/
        │   ├── CMakeLists.txt
        │   └── rtl_dci.c           寄存器配置 + DMA 编排 + 帧状态机 + reg dump（与 SoC 无关）
        └── rtl87x3k/
            ├── CMakeLists.txt
            ├── rtl_dci_def.h       ✅ 寄存器映射 + 位域 union
            ├── rtl_dci_int.h       SoC 钩子声明
            ├── rtl_dci_int.c       ★ 握手索引 / 时钟 / 引脚 / 通道申请 / 中断向量 —— 所有 HAL 占位只在这
            ├── rtl_dci_dma.h       DMA 抽象接口（中性命名）
            └── rtl_dci_dma.c       ★ 唯一出现 DMA_* 名字的文件：描述符 CTL_LOW 组装、通道编程、中断取用
```

**`rtl_dci_dma.{h,c}` 的存在理由**：这一族 SoC 的 DMA 驱动做过一次改名（老片子 `GDMA_*`，这颗 `DMA_*`），而且字段宽度也有差异（`BLOCK_TS` 20 bit vs 16 bit）。把这些全部收进一对文件，`rtl_common/rtl_dci.c` 就完全不 include `rtl_dma.h`、不出现任何 SoC 名字 —— 换 SoC 只需另写一份 `rtl_dci_dma.{h,c}` + `rtl_dci_int.c`，1800 行业务逻辑一行不动。

> 目录名原为占位的 `rtl_8`，**现已定名 `rtl87x3k`** —— DCI 只存在于 BBPro4（RTL87X3K），且该 SoC 的 SDK 已确认存在（见 §2 基址一节）。命名风格与其他 IP 的 `rtl8773g` / `rtl87x2g` 一致。

### 分层职责

| 层 | 依赖 | 内容 |
|---|---|---|
| `rtl_dci.h` | 无 | 类型、枚举、内联寄存器操作。不含 DMA/OS 依赖 |
| `rtl_common/rtl_dci.c` | `rtl_dci_int.h` | 全部业务逻辑。**不 include `trace.h`、不 include `rtl_dma.h`、不用 OS 原语** |
| `rtl87x3k/rtl_dci_dma.c` | `rtl_dma.h` | 描述符 CTL_LOW 组装、通道编程、中断取用、寄存器采集 |
| `rtl87x3k/rtl_dci_int.c` | `rtl_dma.h` `hal_dma.h` `rtl_rcc.h` `os_sync.h` `trace.h` | 握手索引、时钟、引脚、通道申请、临界区、向量表 |

---

## 4. HAL 预留点

目标 SoC 的 SDK 已定位在 `D:\Project\Evaluation\realtek`（RTL87X3K / BBPro4，AP 核）。
**2026-09-24 更新：FPGA 阶段的 HAL 缺口已全部清零。**
DMA 握手索引与 DCI 时钟都已由 SoC 侧给出答案，详见 §17 表格与
`rtl_dci_int.c` 顶部的注释：

- **握手索引 = 48**（不是端口表里的 49）。依据 `BBPRO4_DmaPorts_20260915_v0.xlsx`
  （history: `20260914_v0 Add handshakes for ... AP DCI, SCP DCI, XP DCI`）＋ SoC 侧
  说明「**AP DMA0 被拿掉，请用 AP DMA1 HS[48]**」。表里那一栏是 AP DMA0/1 **合并编号**，
  而 `SRC_PER` 是**每控制器本地端口号**，所以落在 DMA1 的 48。
- **DCI 时钟默认开启**，`rtl_dci_clock_enable()` 保持空实现（真实使能语句注释保留），
  **不再是阻塞项** —— 寄存器块当下即可访问。
- **引脚复用在 FPGA 上不需要配置** —— DCI 是顶层专用 RTL 端口，由 `.xdc` 钉死，
  不经过 SoC pinmux，`rtl_dci_pinmux_init()` 空实现即正确。ASIC 侧待确认。
  详见 §17 项 13。
- 连带修掉两个真 bug：`rtl_dci_dma_clock_enable()` 原来调 `DMA_CLOCK`，而 AP 核上
  `DMA_CLOCK` 被定义成 `NULL`（只有 SCP 核才是 `DMAC0_CLOCK`），整句是**空操作**；
  以及 reg dump 只解码 `src_per` 低 4 位，而 48 的低 4 位是 0，会打印成 `SRC_PER=0`
  并被读成 UART0_TX。

**FPGA 上板前提**（板级，不是驱动配置）：必须用**带 DCMI 的 bitfile** ——
DCI 的脚位在 J4 上复用了 MIPI D-PHY 调试信号与 LCDC 数据线，所以
**DCMI 与 MIPI/LCDC 在该连接器上互斥**。camera 的 I2C 是
`FPGA_IO_PIN_AP[80]/[81]`（J4 `G29`/`G30`），刻意排在 DCI 脚位旁边供 sensor 初始化。

复查中最有价值的一条不是"DCI 缺"，而是**整个 display island 的 HAL 都还没落地**：

| 证据 | 说明 |
|---|---|
| `address_map.h` 有全套基址 | JPEG 0x40640000、MIPI 0x40641000、SPI_JDI 0x40642000、**DCI 0x40643000**、PNG 0x40644000、DISPLAY_PCC 0x40647000、IDU 0x40650000 |
| 向量表有全套中断 | `JPEG_IRQn 179`、`PNG_IRQn 180`、`MIPI_IRQn 181`、**`DCI_IRQn 182`**、`RXI300_DISPLAY_IRQn 189` |
| 但这些基址**在 SDK 里零使用者** | `grep 0x4064 / DISPLAY_PCC` 全树只命中 `address_map.h` 自己 |
| RCC 时钟枚举无 display 项 | AP 核 `RCCClock_TypeDef` 里 DCI/IDU/PPE/MIPI/JPEG/PNG/GPU 全无 |
| pinmux 无 display 类功能 | `pin_def.h` 功能号止于 `SPIC0_SIO3(72)` |

即：硅上有、中断图有，但**时钟/电源/引脚 HAL 对该岛任何一个外设都还没进 tree**。
谁先点亮第一个，其余几个一起解锁 —— 所以这不是 DCI 单独等，而是整岛在等同一份东西。

spec §2.3 确认硬件请求线存在：

> 通过设定 `DCI_CR.CAPTURE` 为 1'b1 来使能 DMA 功能。DCI 每次收到一个完整的 32 位数据块时，都将触发一个 DMA 请求。

但索引号是 SoC 级 mux 值，spec 不会给。全部占位集中在 `rtl_dci_int.c`：

```c
/* 已解决。48 而非端口表里的 49：AP DMA0 被裁，DCI 走 DMA1，
 * 而 SRC_PER 是每控制器本地端口号。 */
#define DCI_DMA_HANDSHAKE_RX    (48U)

uint8_t rtl_dci_dma_handshake_rx(void)
{
    return DCI_DMA_HANDSHAKE_RX;
}
```

> 注意这里是 `DMA_*` 而不是 `GDMA_*`：该 SoC 已把 API 整体改名，且握手索引经
> `rtl_dci_dma_arm()` 写入 `CFG_HI.SRC_PER`，不再是填 `GDMA_InitTypeDef` 结构体。
> 相关差异全部吸收在 `rtl_dci_dma.{h,c}` 一对文件里（§3）。

HAL 到位后只改这一个文件。

---

## 5. 数据通路：GDMA 配置

DCI 自身没有 DMA 引擎，用系统通用 GDMA，**单条通道**（对比 IDU 需要 RX/TX 两条）。

### 固定项（不给用户配，避免配错）

| 字段 | 值 | 理由 |
|---|---|---|
| `GDMA_DIR` | `PeripheralToMemory` | 外设在源端 |
| `GDMA_SourceAddr` | `&DCI->DR` | 固定 |
| `GDMA_SourceInc` | `DMA_SourceInc_Fix` | DR 是固定 FIFO 出口 |
| `GDMA_DestinationInc` | `DMA_DestinationInc_Inc` | 写 framebuffer |
| `GDMA_SourceDataSize` | `GDMA_DataSize_Word` | DR 一次出 byte0~3 |
| `GDMA_DestinationDataSize` | `GDMA_DataSize_Word` | 同上 |
| `GDMA_SourceMsize` | **`GDMA_Msize_1`** | **spec §2.3：请求逐 word 拉。配大 burst 会欠载**。不能照抄 IDU 的 `Msize_8` |
| `GDMA_DestinationMsize` | **按通道 FIFO 深度推导**，见下 | 目的侧是内存、无握手，**不受 spec §2.3 约束**，应攒够多个 word 再突发写 |

#### `SRC_MSIZE` 与 `DEST_MSIZE` 必须区别对待

DW DMAC 的这两个字段是**独立**的：

- `SRC_MSIZE` 决定每次 `dma_req` 从源搬几个 item。DCI 逐 word 拉请求，所以只能是 1。
- `DEST_MSIZE` 决定从通道 FIFO 往内存排空时每次写几个 item。**目的端是内存，没有握手，不受源侧约束。**

把目的侧也配成 1 会让写侧退化成单字访问，白白丢掉 AHB 突发。照 IDU 按通道深度选的做法（`rtl_idu_get_dma_depth()`）：

| 通道 | FIFO 深度 | `DEST_MSIZE` |
|---|---|---|
| ch0 ~ ch1 | 32 | `GDMA_Msize_8` |
| ch2 ~ ch5 | 8 | `GDMA_Msize_8` |
| ch6 ~ ch15 | 4 | `GDMA_Msize_4` |

这个收益与分块粒度无关，且**远大于块数带来的任何差异**（§6.3）——源侧已经是单字访问、没有突发可丢，所以写侧的突发是唯一能优化的地方。

两点已确认不受影响：

- **块尾不足一个 `DEST_MSIZE`**：DMAC 用单次事务补齐，不需要 `BLOCK_TS` 是 `DEST_MSIZE` 的整数倍。
- **「块完成 = 数据已落内存」仍成立**：块完成意味着该块全部数据已写出，FIFO 必然排空，所以 §7.3 的通知时机假设不变。

> `TODO(verify)`：`DEST_MSIZE` 与通道 FIFO 深度的实际配合、以及块尾补齐行为，需上硬件确认。见 §17。
| `GDMA_SourceHandshake` | 见 §4 | 硬件握手 |
| `GDMA_Multi_Block_Mode` | `LLI_WITH_CONTIGUOUS_SAR` | BIT27 = `LLP_DST_EN`，见 §6 |

### 用户可配项

| 字段 | 说明 |
|---|---|
| `dma_channel_num` | **建议 0 或 1**。通道 FIFO 深度不同：ch0-1 深 32、ch2-5 深 8、ch6-15 深 4（见 IDU 的 `rtl_idu_get_dma_depth()`）。DCI 是持续流、溢出即废帧，应占深通道 |
| `dma_priority` | 0 最高，建议给高优先级防溢出 |

---

## 6. 缓冲管理：分块环形 LLI

### 6.1 为什么一律分块

**分块的唯一理由是「单 block 装不下」。** `DCI_MAX_BLOCK_TS = 65,535` word = 262,140 字节（`BLOCK_TS` 字段 **16 bit**）：

| 格式 | 帧字节数 | 最少块数 |
|---|---|---|
| 320×240 RGB565 | 153,600 | **1** |
| 466×466 RGB565 | 434,312 | **2** |
| 640×480 RGB565 | 614,400 | **3** |
| 1280×720 RGB565 | 1,843,200 | **8** |
| 2048×2048 单色 8-bit | 4,194,304 | **17** |
| 2048×2048 RGB565 | 8,388,608 | **33** |

> ⚠️ **`BLOCK_TS` 是 16 bit，不是 20 bit。** 本文档早期版本按 20 bit（1,048,575 word）推导，因为参照的是 rtl87x3g 的 `GDMA_CTL_HIGHx_W_TypeDef.BLOCK_TS: 20`。目标 SoC（RTL87X3K/BBPro4）的 `DMA_CTLx_H_W_TypeDef.block_ts: 16` —— 整个 87x2g/87x2j/87x3d/87x3j/87x3k 系列都是 16 bit，其 `rtl_dma.h` 也写明 `DMA_BufferSize` 范围是 *1 to 65535*。**单块容量小 16 倍**，块数因此普遍上升，`DCI_MAX_CHUNK` 从 3 改为 33（见 §6.3）。
>
> 这条差异只影响块数推导，不影响任何机制 —— 环形拓扑、会合、门控、所有权全部照旧，因为它们本来就不假设块数是多少。

曾经还有两个理由，都已作废：

- ~~帧内流水~~：块中断让消费者提前处理部分帧。已随 §6.3 的「装不下才加块」规则放弃，`chunk_ready` 回调同时删除。
- ~~JPEG 进度可见与越界检测~~：改用单块 + `LLP = 0` + `GDMA_INT_Transfer`，更简单也更安全，见 §9.2。

### 6.2 分块粒度：按行对齐

#### `frame_bytes` 完整公式

**DCI 无法从自身寄存器得知帧多大** —— `CROP=0` 时捕获的是 `HSYNC`/`VSYNC` 界定的任意大小，所以 sensor 几何必须由应用声明（`DCI_InitTypeDef.sensor_pclk_per_line` / `sensor_lines`，用 pclk 而非 pixel，因为 `CAPCNT` 本身是 pclk 计数、且 pclk↔pixel 换算依格式而变）。在此之上 `EDM` 决定每 pclk 几字节，`BSM`/`LSM` 各自抽掉一部分：

```
bytes_per_pclk = (edm == 8BIT) ? 1 : 2          /* 10-bit 每样本占 16 bit，§2 数据打包 */
pclk_per_line  = crop ? crop.w : sensor_pclk_per_line
lines_in       = crop ? crop.h : sensor_lines

bsm_num/bsm_den = { ALL: 1/1, EVERY_2ND: 1/2, 1_OF_4: 1/4, 2_OF_4: 2/4 }
line_bytes     = pclk_per_line * bytes_per_pclk / bsm_den * bsm_num
lines_out      = lsm ? (oels ? floor(lines_in/2) : ceil(lines_in/2)) : lines_in

frame_bytes    = line_bytes * lines_out
frame_words    = ceil(frame_bytes / 4)
```

三点说明：

- **`OELS` 影响行数，`OEBS` 不影响字节数。** `LSM` 下 `oels=0` 取第 0/2/4… 行（`ceil`），`oels=1` 取第 1/3/5… 行（`floor`）；奇数行时两者差一行。`OEBS` 只选抽取组内的起始字节，组数不变。
- **`BSM` 要求行长能被抽取组整除**，否则组会跨行、后续每行整体偏移 —— 校验拒绝（§15）。
- **`BSM` 只在 `EDM=0` 下有效**（寄存器表：*valid only when EDM[1:0]=00*），互斥校验见 §15。

#### 分块

```
line_words       = line_bytes / 4
lines_per_chunk  ← 驱动内部推导，不对外暴露（§6.3）
chunk_words      = lines_per_chunk * line_words
chunks_per_frame = ceil(lines_out / lines_per_chunk)      ≤ DCI_MAX_CHUNK
desc_count       = buf_count * chunks_per_frame           ≤ DCI_MAX_DESC
```

按行对齐而不是按固定字节对齐，因为：块边界对消费者有意义（可按行处理）、与 DCI 的 `LINE` 中断语义一致、不会出现跨行的半块。

一帧的最后一块允许更短（`lines_out % lines_per_chunk != 0`），每个描述符自带 `CTL_HIGH`，天然支持。

**两条约束**：

```c
#define DCI_MAX_BUF     2       /* 最多 2 块 framebuffer */
#define DCI_MAX_CHUNK   33      /* 每帧最多 33 块；由 spec 最大尺寸反推，见 §6.3 */
#define DCI_MAX_DESC    (DCI_MAX_BUF * DCI_MAX_CHUNK)    /* = 66，环共 1320 字节 */
```

- `chunk_words <= DCI_MAX_BLOCK_TS`（65,535），即 `lines_per_chunk <= DCI_MAX_BLOCK_TS / line_words`
- `chunks_per_frame <= DCI_MAX_CHUNK`

**退化情形**：`line_bytes % 4 != 0` 时（无 crop 且宽度使行长非 4 倍数，如 466 宽单色 8-bit）无法按行对齐，退化为按 word 对齐分块。驱动置内部标志并在 dump 里可见（`line_aligned = 0`）。由于块划分已不对外暴露，这个退化对用户不可见，只影响诊断输出。

### 6.3 块数规则：装不下才加，逐级递增

**块数不是可调项，而是由「能不能装下」唯一确定的推导结果。** 规则：

```
从 1 块开始，装不下就 +1，直到装得下：

  n = 1
  while (ceil(lines_out / ceil(lines_out / n)) 的 chunk_words > DCI_MAX_BLOCK_TS)
      n++
  chunks_per_frame = n                      /* 必然 ≤ DCI_MAX_CHUNK = 33 */
  lines_per_chunk  = ceil(lines_out / n)
```

等价的闭式：`chunks_per_frame = ceil(frame_words / DCI_MAX_BLOCK_TS)`，再按行对齐取整。

**绝不为了"帧内流水"主动多切块。** 这条规则的取舍：放弃帧内流水，换来配置面收敛（块数不再是用户可配项）、中断数最小、行为完全可预测。

连带影响已落实：`lines_per_chunk` 与 `jpeg_chunk_bytes` 从 `DCI_DMA_config` 移除、改为内部推导（§14）；`chunk_ready` 回调删除（§11.4）。

推导结果与中断率（每帧另有一次 `FRAME`）：

| 场景 | line_words | frame_words | 块数 | lines/chunk | @fps | 块中断/秒 |
|---|---|---|---|---|---|---|
| 320×240 RGB565 | 160 | 38,400 | **1** | 240 | 30 | 30 |
| 466×466 RGB565 | 233 | 108,578 | **2** | 233 | 60 | 120 |
| 640×480 RGB565 | 320 | 153,600 | **3** | 160 | 30 | 90 |
| 1280×720 RGB565 | 640 | 460,800 | **8** | 90 | 30 | 240 |
| 2048×2048 单色 8-bit | 512 | 1,048,576 | **17** | 121 | — | — |
| 2048×2048 RGB565 | 1024 | 2,097,152 | **33** | 63 | — | — |

**16 bit 的 `BLOCK_TS` 让「绝大多数情况 1 块」不再成立** —— 单块只装 256 KB，常见分辨率就要 2~3 块。中断率随之上升（466×466 @60fps 从 60 次/秒变 120 次/秒），但仍是每帧个位数次，与像素速率相比可忽略；`FCRC` 降帧和门控这些机制都不受影响。

代价评估：

| 项 | 20 bit 假设下 | 16 bit 实际 |
|---|---|---|
| 描述符环 RAM | 6 项 = 120 字节 | 66 项 = **1320 字节** |
| 466×466 @60fps 块中断 | 60 次/秒 | **120 次/秒** |
| 2048×2048 RGB565 块中断 | 3 次/帧 | **33 次/帧** |

1320 字节的静态数组仍然可接受（对比一块 466×466 RGB565 framebuffer 是 424 KB），所以**不引入「块数上限可配」这种旋钮** —— 那会把配置面又打开一道口子，而省下的是 KB 级的 RAM。

#### 为什么上限恰好是 33

由最坏情形反推：spec §2.9 允许的最大是 2048×2048，非 JPEG 格式里每像素最多 2 字节（RGB565 / YCbCr422 / 10-bit raw）。

```
最长行:      2048 像素 × 2 字节 / 4 = 1024 word/行
单块最多容纳: floor(65,535 / 1024) = 63 行            ← BLOCK_TS 16 bit
覆盖 2048 行: ceil(2048 / 63) = 33 块                 ← 恰好 33
```

验算最大配置：`lines_per_chunk = ceil(2048/33) = 63`，`chunk_words = 63 × 1024 = 64,512 ≤ 65,535` ✓，块划分 32 × 63 + 32 = 2048 行 ✓，`ΣCTL_HI = 2,097,152` = `frame_words` ✓（主机侧已验证，见 §18）。

结论：**spec 允许的所有非 JPEG 配置都被覆盖**，`Capture_Init` 不会因为分辨率而拒绝任何合法输入。

### 6.4 描述符结构：CONTINUOUS 闭环 / SNAPSHOT 独立链

单/双缓冲统一为**同一段代码里的 N = 1/2**，用**同一个描述符数组**；两种 capture mode 的差别只在 `LLP` 怎么接。

两种模式共用的 GDMA 模式位是 `LLI_WITH_CONTIGUOUS_SAR`（BIT27 = `LLP_DST_EN`）：DAR 从描述符取，SAR 保持 contiguous —— 因为 `SINC = Fix`，SAR 本来就不动，永远指向 `&DCI->DR`。

#### CONTINUOUS：闭环

`buf_count` 个组、每组 `chunks_per_frame` 个描述符，首尾相接：

```
                DAR = buf[k] + i × chunk_bytes

buf_count=1, chunks=1        buf_count=1, chunks=2
   d0 ──┐                       d0 ──► d1 ──┐
   ▲    │                       ▲           │
   └────┘                       └───────────┘

buf_count=2, chunks=1        buf_count=2, chunks=2      ← 最常用
   d0 ──► d1 ──┐                ┌── buf0 组 ──┐┌── buf1 组 ──┐
   ▲           │                d0 ──► d1 ──► d2 ──► d3 ──┐
   └───────────┘                ▲                          │
                                └──────────────────────────┘
```

`chunks=3` 只在最大分辨率下被强制（§6.3），形状同理，每组三个描述符。

「一个 buffer 满」= 该组最后一个描述符完成（`chunks=1` 时就是唯一那个）。

#### SNAPSHOT 不建闭环，而是 N 条独立链

上面画的闭环是 **CONTINUOUS 模式**的形态。SNAPSHOT 用**同一个描述符数组、不同的 `LLP` 接法** —— 每组以 `LLP = 0` 结尾：

```
CONTINUOUS（闭环）              SNAPSHOT（N 条独立链）
  ┌── buf0 ──┐┌── buf1 ──┐        buf0 链: d0 ──► d1 ──► 0
  d0 ─► d1 ─► d2 ─► d3 ─┐         buf1 链: d2 ──► d3 ──► 0
  ▲                      │
  └──────────────────────┘        Start() → 从 d[组首] 重新装载通道
```

三个好处：

1. **每次 `Start()` 从零重装通道**（`GDMA_Init` 指向 `ring[组首]`），每张都从确定状态开始，不依赖上一张怎么结束的 —— 上一张若 OVR 中途停下也不必 resync。快照是稀疏操作，重装成本可忽略。
2. **`LLP = 0` 让块传满时 DMA 干净停止**并触发 `GDMA_INT_Transfer`。对 JPEG 这就是越界检测（§9.2）；对定长格式这就是"帧数据到齐"。
3. **重装就是重调一次 `GDMA_Init()`**，与初始化同一条路径（IDU 已验证），不需要任何特殊的「从环中途重启」原语。

**`buf_count = 2` 在 SNAPSHOT 下同样有意义**：驱动维护轮转下标，每次 `Start()` 打到下一块，应用可以在处理第 N 张时把第 N+1 张收进另一块（连拍 / 按需抓拍 + 流水处理）。目标 buffer 仍被持有时 `Start()` 返回 `DCI_ERROR_BUSY`（§14）—— 快照是应用显式发起的单次操作，失败快比静默门控好，后者表现为"调了 Start 什么也没发生"，像卡死。因此 `on_busy` 在 SNAPSHOT 下无意义。

应用通过交付时的 `frame.index` 得知这一张落在哪块，不需要预先知道。

#### 中断的可用性随拓扑而变

| 拓扑 | `GDMA_INT_Transfer` | `GDMA_INT_Block` |
|---|---|---|
| CONTINUOUS 闭环（`LLP` 永不为 0） | **永远不触发** | 每块一次，唯一可用信号 |
| SNAPSHOT 独立链（末描述符 `LLP = 0`） | **链走完即触发** —— 定长下是"帧数据到齐"，JPEG 下是"越界" | 每块一次 |

这条差异贯穿 §7.2 的中断策略，是两种模式必须分开讨论的根源。

（`GDMA_INT_Half_Block` 本 SoC 也支持，但块数已收到最小，不需要更细的进度。）

### 6.5 描述符存放

静态数组，放非缓存区（见 §10），**不用 `os_mem_alloc`**（对比 IDU 的做法）：

```c
DCI_NONCACHE_SECTION static GDMA_LLIDef dci_lli_ring[DCI_MAX_DESC];
```

好处：确定性、无分配失败路径、天然满足非缓存要求。`DCI_MAX_DESC = DCI_MAX_BUF × DCI_MAX_CHUNK = 2 × 33 = 66`，**整个环占 1320 字节**（每个 LLI 5 个字），超出在 `Capture_Init` 报错。

### 6.6 为什么不用 auto-reload

`AUTO_RELOAD_WITH_CONTIGUOUS_SAR`（BIT31 = `RELOAD_DST`）看似省内存，但只支持单 buffer，且 `chunks_per_frame > 1` 时 DAR 每块都回到帧首 → **直接错**。分块是本设计的基础，auto-reload 与之不兼容。

### 6.7 核心不变式：一帧完整落在一块 buffer 内

```
buf_bytes ≥ frame_bytes            （§15 强制校验）
```

**一帧的所有块必然落在同一块 buffer 里、地址连续；buffer 切换只发生在 group 边界，不在块边界。** 硬件上每个描述符自带 `DAR`，把一帧拆到两块 buffer 是做得到的，但设计层面禁止 —— 见 §6.8 的评估结论。

这条不变式支撑了本设计的一系列性质，缺了它全部要重做：

| 依赖它的性质 | 出处 |
|---|---|
| `DCI_frame_t{addr, bytes}` 能描述一整帧（单段连续内存） | §11.1 |
| 所有权粒度 = buffer，`ReleaseBuffer(index)` 一次归还一帧 | §11.5 |
| ready 队列可退化成 `ready_head` + `ready_count` 两个字节 | §11.3 |
| **背压在任何配置下都有效**，「忘了 Release 只会停摆、不会损坏数据」 | §8.2 |
| CONTINUOUS 定长下 DMA 全程硬件自转，软件只记账 | §7.2 |

### 6.8 支持的缓冲配置与选型

块数已由 §6.3 的规则唯一确定，**用户能调的只剩两个旋钮**：

```c
buf_count  ∈ {1, 2}                   /* DCI_MAX_BUF = 2 */
on_busy    ∈ {GATE, OVERWRITE}        /* §8.4 */
```

**实际组合只有 6 种**（SNAPSHOT 下 `on_busy` 无意义，见 §6.4），穷举得完。以 466×466 RGB565 @60fps（整帧 424 KB、块数推导为 1）为例：

| 用途 | `buf_count` | `on_busy` | RAM | 消费者余量 |
|---|---|---|---|---|
| 单张抓拍 | 1 | — | 424 KB | SNAPSHOT，`CAPTURE` 硬件自清 |
| **连拍 / 流水抓拍** | **2** | — | 848 KB | SNAPSHOT + 轮转：处理第 N 张时收第 N+1 张（§6.4） |
| 按需逐帧 | 1 | `GATE` | 424 KB | 无限（每帧停等，帧率随消费者） |
| **取景器 / 推理喂帧** | 1 | `OVERWRITE` | **424 KB** | **1 帧周期**（16.7 ms），超时撕裂 |
| 标准流式 | 2 | `GATE` | 848 KB | 1 帧周期，有背压 |

选型要点：

- **块数不是旋钮**（§6.3）。它由「装不下才加」唯一推导，通常是 1。原先「块数 vs 描述符 RAM / 中断率」的权衡不存在了。
- **`buf_count = 1` + `OVERWRITE` 是省 RAM 的首选**（「取景器」那一行）：RAM 只要 1× 帧，deadline 却是**整个帧周期**（DMA 要等下一帧才绕回同一位置），代价只是超时会撕裂、靠 `seq` 可察觉。
- **`buf_count` 提供的余量是 `buf_count − 1` 帧**（§8.1）。上限为 2 意味着**余量最多 1 个帧周期** —— 消费者耗时抖动大时只能靠 `FCRC` 硬件降帧率（§8.3），没有"再加 buffer"或"帧内流水"这两条退路了。

### 6.9 已评估并否决：小于整帧的 buffer 乒乓

曾评估过「两个小于整帧的 buffer 轮流填」的流式方案（RAM 可低至 `O(W)`、与高度无关），**已否决**。硬件本来支持（描述符环结构不用改），否决理由在语义层：

| 否决理由 | 说明 |
|---|---|
| **独占区间很窄** | 只有「总 RAM 装不下一整帧」时才有价值。RAM ≥ 1 帧时被 §6.8 的「取景器」配置完全支配 —— 同样 1× 帧 RAM，那个方案还多给连续整帧和 2 倍宽松的 deadline |
| **无背压，且失效被放大** | 帧内没有安全停顿点：清 `CAPTURE` 后 DCI 仍会跑完当前帧（spec §2.5.2），DMA 无处可写 → OVR → **损失整帧 + 触发 resync**。消费者晚一个 buffer 周期就废一帧 |
| **破坏核心安全性质** | 它是本设计所有安全保证的例外，混进来会削弱「忘了 Release 只会停摆」这条最重要的性质（§6.7） |
| **帧不连续** | 任何需要回看整帧的处理都得重新组帧，而组帧就把省下的 RAM 还回去了 |
| **行数整除约束刁钻** | 免掉 per-frame re-arm 需 `lines_out % lines_per_buf == 0`。而 466 = 2 × 233、233 为质数 → 只能取 1/2/233/466 行，中间无可选项；JPEG MCU 的 16 行也不整除（`466/16 = 29.125`） |

**重新考虑的门槛**：出现「总 RAM < 一整帧」的真实需求（典型是 ≥720p，或硬件 JPEG 编码器直连），且消费者是严格单向流式的硬件模块。届时底层机制（描述符环、硬件切换、`FRAME` 权威判定、resync）全可复用，改动集中在语义层：新增模式位、`DCI_frame_t` 加 `seg_idx`/`seg_total`、流式模式下禁用 `on_busy`、不整除时复用 JPEG 的 re-arm。

---

## 7. 帧同步与失步处理

### 7.1 问题

**DMA 只会数 word，完全不知道帧边界。** 两者对齐的前提是每个 DMA 请求都被搬走了。spec 明确有两处会破坏这个前提：

1. **溢出**（spec §2.8）：FIFO 无溢出保护，超速时数据被覆盖 → word 丢失 → DMA 永久落后 → 之后每帧都在 buffer 里错位。
2. **裁剪提前结束**（spec §2.6）：`CWSIZE` 的行数还没到，VSYNC 就先有效 → capture 停止并产生 `IT_FRAME`。这一帧 word 数偏少 → 当前块完不成 → 下一帧继续填同一块，失步。

DCI 侧能自愈（spec §2.8：同步出错或溢出后 FIFO 复位、等新帧开始），**驱动只需重同步 DMA 侧**。

### 7.2 `IT_FRAME` 三种情形下都恒开

#### 前置：`RIS` 与 `IER` 无关，会 latch

spec §3：

> `DCI_RIS`：只读寄存器，在通过 `DCI_IER` 寄存器掩蔽相应中断之前给出该中断的当前状态（**无论 `DCI_IER` 寄存器是否使能**）

即 `MIS = RIS & IER`，而 `RIS` 位置起不受 `IER` 影响，只能靠 `ICR` 写 1 清。两个推论：

- 屏蔽期间发生的 `FRAME` 不会丢 —— 事后开 `IER.FRAME` 会立刻触发（电平型）。**ST 的动态开关技巧完全依赖这一点。**
- 但 `RIS` 是**单 bit，不是计数器** —— 屏蔽期间来了 2 次和来了 1 次无从区分。这决定了连续模式不能屏蔽它。

#### 「帧完成」的信号源随模式而变

| | 描述符结构 | `GDMA_INT_Transfer` 的含义 | 正常完成靠什么 |
|---|---|---|---|
| SNAPSHOT 定长 | N 条链，`LLP = 0`（§6.4） | 块传满 = **数据超量**（帧偏长） | `IT_FRAME` |
| SNAPSHOT JPEG | 单块，`LLP = 0`（§9.2） | 块传满 = **越界** | `IT_FRAME` |
| CONTINUOUS JPEG | 单块，`LLP = 0`（§9.4） | 块传满 = **越界** | `IT_FRAME` |
| CONTINUOUS 定长 | 闭环，`LLP ≠ 0` | **永不触发** | `GDMA_INT_Block` ∧ `IT_FRAME`（会合） |

`LLP = 0` 的三种情形里，块传满都不是正常路径 —— 定长下 `BLOCK_TS` 恰为帧字长，传满还没收到 `FRAME` 就说明 sensor 输出超出声明；JPEG 下帧长小于容量，传满即越界。

#### 结论：三种情形 `IT_FRAME` 全部恒开

| | `IT_FRAME` | `GDMA_INT_Transfer` | `GDMA_INT_Block` | 交付时机 |
|---|---|---|---|---|
| **SNAPSHOT 定长** | 恒开 = **完成** | 恒开 = **数据超量告警** | — | FRAME（等 `FIFO_EMPTY` 后读 `CURR_DAR`） |
| **SNAPSHOT JPEG** | 恒开 = **完成** | 恒开 = **溢出** | — | 同上（同一条代码路径） |
| **CONTINUOUS JPEG** | 恒开 = **完成** | 恒开 = **溢出** | — | 同上 + 驱动自动重启（§9.4） |
| **CONTINUOUS 定长** | 恒开（会合 + 偏短看门狗） | 永不触发 | 恒开 | **组末块完成 ∧ FRAME 已到**（会合） |

**只有 CONTINUOUS 定长需要会合。** 另外三种都是 `LLP = 0`、DMA 停在原地、`CURR_DAR` 可信，`IT_FRAME` 一个事件就能定论。会合是专为「闭环 + 无 `Transfer`」这一种情形准备的。

**ST 的动态开关一处都不需要**：SNAPSHOT 有 `Transfer` 作配对信号，比使能窗口更直接；CONTINUOUS 需要计数 `FRAME`，一屏蔽就丢失计数能力。ST 只有 DBM 的计数器绕回可用，才不得不靠使能窗口。

注意 `Transfer` 在两种 SNAPSHOT 下语义**互为镜像** —— 定长下"块传满 = 帧齐"是正常事件，JPEG 下是溢出错误。

#### 各情形的 ISR 逻辑

**SNAPSHOT（定长与 JPEG 共用一条路径）** —— **不用会合，`IT_FRAME` 本身就是结论性的。**

快照模式下两个条件同时成立，而连续模式下都不成立：

- **`CAPTURE` 硬件自清** → 收到 `FRAME` 即确知不会再有数据进来
- **`LLP = 0`，DMA 停在原地** → `CURR_DAR` 减基址就是实收字节数，不会被环前进污染

所以一个事件 + 一次寄存器读即可定论：

```
IT_FRAME（capture 已结束）:
    等 CFG_LOWx.FIFO_EMPTY                        /* §9.3 */
    received = xfer_seen ? 块满量（定长=frame_bytes，JPEG=buf_bytes）
                         : CURR_DAR - buf[k]
    定长: received == frame_bytes → 正常交付
          received <  frame_bytes → 偏短，交付 truncated + FRAME_SHORT
    JPEG: 无期望长度，received 即帧长，正常交付
    GDMA_Cmd(DISABLE)                             /* 中止可能未完成的块 */

GDMA_INT_Transfer（块传满）:
    if (DCI->RIS & FRAME)     /* RIS 会 latch，用它解竞态而非超时 */
        → 正常，交给 FRAME 路径处理
    else
        → 数据超出容量：CAPTURE = 0
          定长 → 交付 truncated + FRAME_LONG
          JPEG → 交付 truncated + BUF_OVERFLOW（§9.2）
```

两种格式只差「期望长度」这一个概念：

| | 期望长度 | `received <` 期望 | `Transfer` 先于 `FRAME` |
|---|---|---|---|
| 定长 SNAPSHOT | `frame_bytes` | `FRAME_SHORT` | `FRAME_LONG` |
| JPEG SNAPSHOT | 无（任意长度合法） | —— | `BUF_OVERFLOW` |

「帧偏长」尤其有用：它就是 **应用声明的 sensor 几何小于实际输出**（§12.6-D1：DCI 无法自己得知帧多大，只能由应用声明），是集成阶段最常见的错误。在这里直接报出来，而不是让用户对着随后必然发生的 OVR 猜原因。

> **为什么快照不用会合。** 会合能免超时，靠的是「缺失会被下一次同类事件自动揭示」（同一 bit 连续置两次）。快照只拍一张，没有"下一次" —— 偏短时 `Transfer` 永不到来，`evt` 会永久停在 `EVT_FRAME`，既不交付也不报错，只能靠**超时**发现，而超时长度没有非任意的答案。反过来，正是"拍完就停"这件事让 `CURR_DAR` 变得可信，于是根本不需要等第二个事件。

**CONTINUOUS 定长** —— **交付由「组末块完成」触发，但必须与 `IT_FRAME` 会合。**

为什么交付点是块完成而不是 `FRAME`：`FRAME` 只说明像素流结束，最后几个 word 可能还在 DMAC 通道 FIFO 里；只有块完成才代表 `BLOCK_TS` 个 word 全部落内存（§7.3）。而 SNAPSHOT 那套「等 `FIFO_EMPTY` 再读 `CURR_DAR`」在这里不可用（环随时已前进到下一组，见下方注）。

正常时序（`chunks_per_frame = 1`）：

```
① IT_FRAME          DCI：像素流结束（此刻 DMA 可能还在排 FIFO）
② GDMA_INT_Block    DMA：BLOCK_TS 个 word 全部落内存   ← 交付在这里
```

`FRAME` 通常先到（DCI 在帧边界立即上报，DMAC 还需至少一次总线事务），**但实现不依赖这个顺序** —— DCI 与 GDMA 是两个中断向量，优先级和延迟都不可控。用每帧一次的会合：

```c
volatile uint8_t evt;        /* bit0 = FRAME 已到，bit1 = 组末块已到 */

DCI FRAME ISR:
    清 ICR.FRAME
    if (evt & EVT_FRAME)          /* 上一次 FRAME 还没配对成功 */
        → 帧偏短：那一帧的组没做完 → DCI_ERROR_DESYNC → resync；evt = 0；return
    evt |= EVT_FRAME ; rendezvous()

GDMA_INT_Block（chunk_idx 递增到 chunks_per_frame，即组末块）:
    if (evt & EVT_BLOCK)          /* 上一次块还没配对成功 */
        → 帧偏长：DCI 未报帧尾 → DCI_ERROR_DESYNC → resync；evt = 0；return
    evt |= EVT_BLOCK ; rendezvous()

rendezvous():                     /* 在 rtl_dci_lock() 临界区内，§11.6 */
    if (evt == (EVT_FRAME | EVT_BLOCK))
        evt = 0
        交付 / lock(buf) / seq++ / 门控判断
```

三个性质：

- **与两个 ISR 的执行顺序无关** —— 谁先到都只置一个 bit，凑齐才交付。
- **两个方向的失配都能抓到** —— 同一个 bit 被连续置两次，说明对面那半从未到过。
- **首帧不误报** —— `evt` 初值 0。

偏短能被抓到的根本原因是 spec §2.6 保证了**即使帧偏短也会产生 `IT_FRAME`**（「行数尚未达到之前 VSYNC 就先变为有效，capture 动作会停止……会产生 IT_FRAME 中断」）。所以偏短表现为「连续两次 `FRAME` 之间没有块完成」，而不是无声无息 —— 这弥补了 ST 动态开关在 `chunks_per_frame == 1` 时的盲区（块永远完不成 → 永远不开 `FRAME` → 既不交付也不报错，静默挂死）。

> **为什么连续模式不能靠 `CURR_DAR` 判断收了多少**：若块恰在帧尾完成，环已前进到下一组，`CURR_DAR` 已指进 `buf[k+1]`，减基址得到垃圾。这一招只在 SNAPSHOT（`LLP=0`，DMA 停住不动）下成立 —— 这正是 §9.3 只把它用于 JPEG 快照的原因。

#### 7.2.1 偏短 / 偏长的残留数据：统一按 `truncated` 交付

三种「干净前缀但不完整」的情形走**完全相同**的交付路径，与 §9.2 的 JPEG 越界一致：

```
等 CFG_LOWx.FIFO_EMPTY
lock(buf[k])
交付 frame{ addr = buf[k], bytes = 实际字节, truncated = true }
frame_error(错误码, ris)
state = ERROR
```

| 情形 | 触发 | `bytes` | 错误码 |
|---|---|---|---|
| JPEG 超出容量 | Transfer 先于 FRAME（`RIS.FRAME` 未置） | `buf_bytes` | `DCI_ERROR_BUF_OVERFLOW` |
| 定长帧偏长 | Transfer 先于 FRAME（同上） | `frame_bytes` | `DCI_ERROR_FRAME_LONG` |
| 定长帧偏短 | FRAME 时 `CURR_DAR − buf[k] < frame_bytes` | `CURR_DAR − buf[k]` | `DCI_ERROR_FRAME_SHORT` |

统一的理由和 §9.2 相同：数据是干净前缀（调试和部分解码都用得上），且**所有权必须有归属** —— 只有帧交付路径会 `lock()`，否则这块 buffer 无主，下次 `Start()` 轮转到它时会在用户还在读时覆盖。

**SNAPSHOT 出错时同样轮转 `cur_group`**（实现时补充）：截断帧照常交付、照常被消费者持有，所以重拍应该打到另一块 buffer，而不是死等这一块被释放。成功与失败走同一条轮转逻辑，少一个特例。

**判据是「DMA 是否已停住」，而不是 capture mode。** 能交付的前提是 buffer 里恰好是干净前缀，这要求 DMA 不再往里写：

| 情形 | DMA 停住？ | 交付截断帧 |
|---|---|---|
| SNAPSHOT 定长 / JPEG | ✅ `LLP = 0` + `CAPTURE` 硬件自清 | ✅ |
| **CONTINUOUS JPEG** | ✅ `LLP = 0` + 驱动防御性关 `CAPTURE`（§9.4） | ✅ |
| CONTINUOUS 定长 | ❌ 闭环仍在转 | ❌ 只 resync + 记 `DESYNC` |

CONTINUOUS 定长不交付的原因：环还在转，下一帧的数据已经续着写进同一块，buffer 里是 `[帧 N 残片][帧 N+1 残片]` 的混合物，不是干净前缀。

`resync` 路径（等价于 ST 的 abort + 应用重启，只是做进驱动）：

```
CaptureCmd(DISABLE) + 回读轮询到 0   /* ISR 档上限，见 §8.2 */
GDMA_Cmd(ch, DISABLE)              /* 内含 safe suspend */
DCI_Cmd(DISABLE) → DCI_Cmd(ENABLE) /* 复位 FIFO */
chunk_idx = 0 ; evt = 0
stats.resync++ ; stats.frames_dropped++
if (GATE && buf[0] 仍被持有)         /* 复用 §8.2 的门控，不覆盖用户数据 */
    cur_group = 0 ; state = SUSPENDED
else
    DMA re-arm 到 desc[0] + CaptureCmd(ENABLE)   /* 硬件自己等下一个 VSYNC */
    state = BUSY
```

**resync 必须先过门控检查**（实现时补充）：resync 无条件回到 group 0，而 buf0 可能正被消费者持有。直接重启会在用户读的同时覆写它 —— 这会破坏 §6.7 的「忘了 Release 只会停摆、不会损坏数据」。复用已有的门控分支即可，不需要新机制；`ReleaseBuffer()` 照常把它接回 BUSY。

**不尝试「续接」**：计数型 DMA 对丢字的流没法可靠续接，拆掉重来是唯一正确的恢复。ST 同样如此（`HAL_DCMI_IRQHandler` 里 OVR/ERR 一律 `HAL_DMA_Abort_IT`）。

### 7.3 通知时机：必须确保数据已落内存

`IT_FRAME` 只说明像素流结束，最后几个 word 可能还在 DMAC 通道 FIFO 里。所以通知前必须有一道「数据已完整落内存」的保证，各模式的手段不同：

| 模式 | 保证手段 |
|---|---|
| SNAPSHOT 定长 / JPEG、CONTINUOUS JPEG | `IT_FRAME` 后**轮询 `CFG_LOWx.FIFO_EMPTY`**，再读 `CURR_DAR`（§9.3） |
| CONTINUOUS 定长 | **组末块完成**即代表 `BLOCK_TS` 个 word 全部写出，再与 `IT_FRAME` 会合（§7.2） |

---

## 8. 背压：`CAPTURE` 门控

### 8.1 为什么不做「丢弃区」

双 buffer 下消费者迟到的余量只有 **1 个帧周期**（不是 2 个）：

```
t0      buf0 组完成 → 通知消费者（帧 N）
t0..t1  DMA 填 buf1（帧 N+1）        ← 消费者的全部余量
t1      DMA 开始重写 buf0（帧 N+2）
        若消费者还在读 → 上半部帧 N，下半部帧 N+2
```

注意后果是**撕裂，不是错位** —— 环形 LLI 是固定硬件结构，DAR/`BLOCK_TS` 不变，帧边界与 buffer 边界始终对齐。

考虑过的丢弃方案及否决理由：

| 方案 | 否决理由 |
|---|---|
| 第三块全尺寸 scratch | 已被 `DCI_MAX_BUF = 2` 排除；即便允许，有这个 RAM 也不如直接加一块正式 buffer（不丢帧），被支配 |
| 重定向描述符 DAR 到另一块 buffer | **LLI 预取**：DW DMAC 在块结尾就取下一个描述符，最早干预时机可能已太晚。要安全就得提前一整块悲观决策再乐观改回，引入 ISR/线程竞态 |
| 丢弃描述符设 `DINC = Fix` 指向 4 字节黑洞 | RAM 只花 4 字节（`DINC` 在 `CTL_LOW[8:7]`，属描述符一部分），但 AHB 写流量一分不省，且同样受预取问题影响 |
| 挂起 GDMA 通道 | DCI 继续产数据 → FIFO 憋爆 → OVR → 每次迟到都触发重量级 resync |

### 8.2 采用方案

利用 spec §2.5.2：

> 如果想要停止捕获，用户需要手动清零 `DCI_CR.CAPTURE`，此时 DCI 会继续抓取数据，直至当前帧结束。

```
buffer 组完成:
    lock(bufN)
    交付帧(bufN)                        /* CALLBACK 模式在此调 frame_ready */
    ─────────────────────────────────  ← 门控检查必须在交付返回之后，见下
    if (下一个要写的 buffer 仍被 lock)
        清 CAPTURE + 回读轮询到 0        /* 当前帧结束后停在帧边界 */
        state = SUSPENDED
        stats.suspend_count++

DCI_Capture_ReleaseBuffer(N)  [线程或 ISR 上下文皆可，§11.5]:
    unlock(bufN)
    if (state == SUSPENDED)
        置 CAPTURE                     /* 硬件自己等下一个 VSYNC，无需回读 */
        state = BUSY
```

**门控检查必须放在交付返回之后。** 消费者可以在 `frame_ready` 回调里就地处理完并同步 `ReleaseBuffer()`（§11.5 允许 ISR 上下文），此时门控检查看到的是已释放状态 —— 这正是想要的：消费者瞬间处理完，不该暂停采集。反过来（先检查后交付）会白白停一帧。

| 性质 | 结果 |
|---|---|
| 额外 RAM | 0 |
| 改描述符 | 不改 → **无预取竞态** |
| 错位 | 无。DMA 停在下一组块首、0 word 已传；恢复后从块首继续 |
| FIFO 溢出 | 无。源头不产数据 |
| 带宽 / 功耗 | 省。丢弃方案照样跑满 AHB，门控一个字节不搬 |

**这个机制被 ST 验证过**：`HAL_DCMI_Suspend()` / `HAL_DCMI_Resume()` 就是清/置 `CAPTURE`，且 `Resume` 里**一个字的 DMA 操作都没有**，证明 DMA 通道原地待命、恢复后对齐不变。

清 `CAPTURE` 后必须**回读轮询到 0** 才算帧结束，这也是 ST 的做法（`HAL_DCMI_Suspend` / `HAL_DCMI_Stop` 都是 `while ((CR & CAPTURE) != 0)`）：

```c
DCI->CR &= ~DCI_CR_CAPTURE;
do {
    if (count-- == 0) { err = DCI_ERROR_TIMEOUT; break; }
} while ((DCI->CR & DCI_CR_CAPTURE) != 0);
```

#### 轮询上限分两档：线程一档，ISR 一档

实现时发现这个轮询在 ISR 里有风险：spec §2.5.2 的"直至当前帧结束"在最坏情况下就是**一整个帧周期**，而门控、resync、连续 JPEG 收尾都在 ISR 里清 `CAPTURE`。ISR 里自旋一帧（16.7 ms @60fps）不可接受。

分档解决，语义不变：

| 上下文 | 上限 | 理由 |
|---|---|---|
| 线程（`Stop` / `Suspend` / `Reconfig` / `SetBuffers` / `Start`） | 长（覆盖一整帧） | **必须**确认停在帧边界才能写配置寄存器（§12.1），等得起 |
| ISR（门控 / resync / 连续 JPEG） | 短（仅覆盖垂直空白期） | 这些路径的调用点**本身就在帧边界上**，正常情况下立即回读到 0；超时也只是"下一帧结束时才真正停"，`CAPTURE` 已经清掉了，门控要的效果不变 |

即 ISR 一档是**尽力确认**而非强制等待，超时不报错、不改变状态机走向。代码里是 `dci_capture_off(max_iter)` 的参数。

### 8.3 配套旋钮

- **`CR.FCRC` 硬件降帧率**：消费者长期偏慢时配成隔帧/隔四帧采集（省 50%/75% 带宽），比不可预测丢帧体面。
- **`buf_count = 2`**：把余量从 0 提到 1 个帧周期。`DCI_MAX_BUF = 2` 封顶，**没有"再加一块 buffer"这条退路** —— 消费者跟不上时只剩 `FCRC` 一条。
- **撕裂检测兜底**：`OVERWRITE` 下消费者靠 `seq` 跳号发现自己错过了帧（§11.1）。

### 8.4 冲突策略必须显式选择

把 §8.2 的门控规则套到 `buf_count = 1` 上会发现：buf0 完成 → 交付 → lock → 下一个要写的还是 buf0，已被 lock → **必然门控**。也就是单 buffer 连续模式实际变成「每帧停等消费者」，帧率被消费者拖着走。

这未必是调用方想要的。单 buffer 的典型诉求恰恰是「我只要最新一帧、不在意撕裂」（取景器、AI 推理喂帧），期望全速跑。所以**不能让 N=1 隐式退化成停等**，要给出显式策略位：

```c
typedef enum {
    DCI_ON_BUSY_GATE      = 0,   /* 默认：门控停等，绝不撕裂，帧率随消费者 */
    DCI_ON_BUSY_OVERWRITE = 1,   /* 照写覆盖，全速跑，可能撕裂 */
} DCI_ON_BUSY_T;
```

`OVERWRITE` 就是 §8.1 分析过的方案 (a)：**只撕裂、不错位** —— 环形 LLI 是固定硬件结构，`DAR`/`BLOCK_TS` 不变，帧边界与 buffer 边界永远对齐，所以覆盖的后果限于「一帧的上下半部来自不同帧」，绝不会出现数据错位或串帧。消费者可通过 `seq` 跳号察觉，`frames_dropped` 也会计数。

两种语义都合法，取舍是「完整性 vs 吞吐」，只有调用方知道该要哪个：

| | `GATE`（默认） | `OVERWRITE` |
|---|---|---|
| 帧完整性 | 保证 | 可能撕裂 |
| 帧率 | 受消费者限制 | 全速 |
| 忘记 `Release` 的后果 | 采集暂停（可观测） | 持续覆盖（靠 `seq` 察觉） |
| 典型场景 | 存储、编码、逐帧分析 | 取景器、推理喂帧 |

`OVERWRITE` 下 §8.2 的门控分支整个跳过，`ReleaseBuffer()` 只清 lock 位、不触发 resume。

---

## 9. JPEG

### 9.1 约束

spec §2.7：`CR.JPEG=1`；**不支持 crop、不支持内嵌码同步**（仅硬件同步）；无尺寸限制；`VSYNC` 启动捕获、`HSYNC` 当数据使能；流长非 4 倍数时尾部**补零**。

难点：**帧长事先未知**，而 `BLOCK_TS` 必须事先给定。

### 9.2 块划分：恒为 1 块，`BLOCK_TS` = buffer 容量

按 §6.3 的规则（装不下才加块），JPEG buffer 再大也是 **1 块** —— 4 MB 以上的 JPEG 不现实。

```
chunk_words   = buf_bytes / 4          /* BLOCK_TS = 整个容量 */
chunks_per_buf = 1
desc_count    = buf_count * 1
```

**链尾必须 `LLP = 0`，不能自环。** 这是 JPEG 超出容量时唯一能保住数据的接法：

| 接法 | JPEG > `buf_bytes` 时 |
|---|---|
| 自环 `LLP = &d0` | 块传满 → DMA 立刻绕回 `buf0 + 0` → **静默覆盖 JPEG 头部**，ISR 再快也来不及 |
| **`LLP = 0`** | 块传满 → **DMA 停止**，`GDMA_INT_Transfer` 触发 → 干净可检测，一字节未损坏 |

于是越界检测就是一个中断。**已收到的数据不丢弃 —— 按截断帧交付给用户**：

```
GDMA_INT_Transfer 触发（块传满）而 IT_FRAME 未到:
    CAPTURE = 0                       /* FRAME 没来，需手动清 */
    等 CFG_LOWx.FIFO_EMPTY            /* 同 §9.3 */
    lock(buf[k])
    交付 frame{ addr = buf[k], bytes = buf_bytes, truncated = true }
    frame_error(DCI_ERROR_BUF_OVERFLOW, 0)
    stats.buf_overflow++
    state = ERROR
```

**为什么要交付而不是丢弃**：块传满意味着 buffer 里是 JPEG 码流**干净的前 `buf_bytes` 字节**（一个前缀，不是被打乱的数据）。JPEG 逐 MCU 行解码，截断流通常仍能解出图像上半部分；调试时也想看到实际收到了什么。丢掉它是驱动替用户做了决定。

更关键的是**所有权必须有归属**：只有帧交付路径会 `lock()`。若只发 `frame_error` 而不交付，这块 buffer 处于无主状态，下一次 `Start()` 轮转到它时会在用户还在读的时候覆盖掉。走正常交付路径后，用户必须 `ReleaseBuffer()`，`Start()` 在它被持有期间返回 `DCI_ERROR_BUSY` —— 和常规流程完全一致，不需要额外规则。

因此这一路会**触发两个回调**：`frame_ready`（带数据和所有权）+ `frame_error`（说明原因）。POLL 模式下截断帧同样进 ready 队列，用户经 `GetFrame()` 取到并看到 `truncated == true`。

#### 只有 BUF_OVERFLOW 值得交付

其余错误的残留数据都是废的，不交付、不 `lock`：

| 错误 | 残留数据 | 是否交付 |
|---|---|---|
| `BUF_OVERFLOW` | 干净前缀 | **✅ 交付，`truncated = true`** |
| `FRAME_LONG` / `FRAME_SHORT`（SNAPSHOT，§7.2.1） | 干净前缀 | **✅ 同一条路径** |
| `OVERRUN`（FIFO 溢出） | 中途丢字 → 图像错位/串行 | ❌ |
| `SYNC`（内嵌码错序） | 帧结构已乱 | ❌ |
| `DESYNC`（CONTINUOUS 偏短偏长） | 环仍在转，已是两帧残片混合 | ❌ |

另外：**`BUF_OVERFLOW` 只可能发生在 JPEG。** 定长格式下 `buf_bytes ≥ frame_bytes` 是不变式（§6.7），`BLOCK_TS` 恰为帧字长，块传满就等于帧完成，不存在越界。

注意 `GDMA_INT_Transfer` 的可用性取决于**拓扑而非格式**（§6.4）：只有 CONTINUOUS 定长的闭环里它永不触发；三种 `LLP = 0` 的情形（两种 SNAPSHOT + CONTINUOUS JPEG）都要使能它，且在这三种里它都表示**数据超量**，不是正常完成。

### 9.3 帧长：`CURR_DAR` 减基址，但必须先等 FIFO 排空

```c
/* 1. 等通道 FIFO 排空 —— 不能跳过 */
while ((chan->GDMA_CFG_LOWx & GDMA_FIFO_EMPTY) == 0) { /* timeout 保护 */ }

/* 2. 读长度 */
bytes = chan->GDMA_CURR_DARx - (uint32_t)buf[k];
```

**先等 `FIFO_EMPTY` 是必要前置条件。** `IT_FRAME` 表示像素流结束，但此刻最后几个 word 可能还在 DMAC 通道 FIFO 里没落到内存，直接读 `CURR_DAR` 会偏小。

- `FIFO_EMPTY` 是 `CFG_LOWx` 的 **bit 9**（只读，复位 1），SDK 里也有 `GDMA_FIFO_STATUS` 宏
- 等价做法是调 `GDMA_SafeSuspend()`（IDU 的 `rtl_idu_wait_dma_idle()` 就是包它）

选 `CURR_DAR` 而非 `CTL_HIGHx` 读回的 `TRANS_DATA_CNT`：减基址语义无歧义，不需要换算宽度或累加已完成块。（ST 用的是等价的「剩余计数」法 `__HAL_DMA_GET_COUNTER()` 读 `NDTR` 剩余，那是 STM32 DMA 的形态。）

#### 补零不裁剪

spec §2.7：流长非 4 倍数时 DCI 在尾部补零，所以上报长度可能多带 1~3 个零字节。

**驱动如实上报 DMA 实际搬运量，不做裁剪。** 理由：驱动只报事实，裁剪是解析策略；且 JPEG 以 `FF D9`（EOI）结尾，消费者要精确长度可自行从尾部回扫，成本是最多 3 字节。

### 9.4 连续 JPEG = 驱动内循环的快照

帧尾不在块边界上，硬件走不到下一块，所以连续 JPEG**每帧都必须软件介入**。但它不需要第三套机制 —— **归约成「快照路径 + 驱动自动重启」**：

| | SNAPSHOT JPEG | CONTINUOUS JPEG |
|---|---|---|
| 每帧收尾 | `IT_FRAME` → 等 `FIFO_EMPTY` → 读 `CURR_DAR` → 交付 | **完全相同** |
| 越界处理 | `Transfer` → `BUF_OVERFLOW` | **完全相同** |
| 重装通道 | 应用调 `Start()` | **驱动在 ISR 里自己做** |
| `CAPTURE` | 硬件自清 | **软件显式关 / 开** |

差别只有「谁触发下一次」。

#### 每帧的 ISR 流程

```
IT_FRAME:
    ① CAPTURE = 0 + 回读轮询到 0          ← 防御性先关，见下
    ② 等 CFG_LOWx.FIFO_EMPTY
       bytes = CURR_DAR - buf[k]
       lock(buf[k]) ; 交付 ; seq++
    ③ GDMA_Cmd(DISABLE)                   中止未完成的块
    ④ k = (k + 1) % buf_count
       if (buf[k] 仍被持有)
           state = SUSPENDED               ← 门控，等 ReleaseBuffer() 再续（§8.2）
       else
           GDMA_Init(DAR = buf[k], BLOCK_TS = buf_bytes/4, LLP = 0)
           CAPTURE = 1                     ← 硬件自己等下一个 VSYNC
```

第 ④ 步让**背压变得比定长格式更自然**：门控判断本来就要决定"要不要把 `CAPTURE` 置回去"，和 re-arm 是同一个决策点，不需要额外分支。

#### 第 ① 步：防御性关 `CAPTURE`，不赌空白期

**连续模式下 `CAPTURE` 不会自清**，所以 `IT_FRAME` 之后 DCI 会在下一个 VSYNC 就开始收帧 N+1。驱动必须在**垂直空白期内**完成 re-arm，否则帧 N+1 的头几个 word 到来时 DMA 还没装好 → FIFO 溢出 → OVR。

这是连续 JPEG **独有**的实时约束 —— 定长连续模式的环是闭合的、DMA 从不停下，没有这个窗口。

| | 赌空白期（不关 `CAPTURE`） | **防御性关（采用）** |
|---|---|---|
| re-arm 赶得上 | 不丢帧 | **不丢帧** —— 在下一个 VSYNC 前置回 `CAPTURE`，spec §2.5.2 说置 1 后接口等帧起始，那个 VSYNC 照样起 |
| re-arm 赶不上 | **OVR**，废帧 + resync | **干净跳过一帧** |

好的情况一样，坏的情况从「错误」降级为「丢帧」，严格更优。而且复用的就是 §8.2 已被 ST（`HAL_DCMI_Suspend`）验证过的 `CAPTURE` 门控机制。

#### 帧长判定沿用快照的机制，不用会合

`LLP = 0` 且块未完成 → DMA 停在原地、无环可前进 → `CURR_DAR` 减基址有效，与 SNAPSHOT 同理（§7.2）。所以连续 JPEG **不需要 §7.2 的 `evt` 会合** —— 那是给「闭环、无 `Transfer`」的定长连续模式准备的。

#### 代价：帧率受 ISR 速度限制

定长连续模式是环形硬件自转、软件只记账，帧率与软件无关。连续 JPEG 每帧都要软件介入一次，**空白期短的 sensor 上可能达不到满帧率**。

缓解手段是现成的：`CR.FCRC` 硬件隔帧采集（§8.3），把可用窗口变相拉长一倍。

> **跳帧不可观测**：re-arm 错过空白期而跳掉的帧，驱动无从知晓（`CAPTURE` 关着，不会产生 `IT_FRAME`），`seq` 只计已交付帧、不会跳号。应用若需要知道 sensor 的真实帧率，注册 `vsync_event`（§11.4）自行计数 —— `VSYNC` 是引脚跳变，与 `CAPTURE` 无关。

### 9.5 与 buffer 模式正交

传输模式由 `init->jpeg` **内部推导**，不给用户配：

```c
typedef enum {
    DCI_XFER_FIXED_LEN = 0,   /* 非 JPEG：一组 = 一帧，帧长已知 */
    DCI_XFER_VAR_LEN   = 1,   /* JPEG：一组 = 一个 buffer 容量，帧长由 FRAME 定 */
} DCI_XFER_MODE_T;
```

`buf_count`（1/2）与之正交，JPEG 一样能双缓冲。用户看到的接口完全一致。

ST 的 HAL 完全不提供 JPEG 长度接口，应用自己在 `FrameEventCallback` 里读 —— 我们把它做进驱动。

---

## 10. 内存归属

### 10.1 分界：驱动只管自己的内存

| 内存 | 归属 | 驱动的责任 |
|---|---|---|
| **framebuffer** | 调用方分配并持有 | **只用，不校验所在区域**。驱动不关心它是否可缓存、在 SRAM 还是 PSRAM |
| **LLI 描述符环** | 驱动内部静态（§6.5） | **驱动自己保证可用** —— 这是模块自身正确性的一部分 |

驱动**不做任何 cache 维护、不校验用户 buffer 所在区域**。理由：buffer 的大小、位置、与下游模块（PPE/LCDC/编码器）的共享关系都只有应用知道；内存区属性是系统级决定（MPU / 链接脚本），驱动越过这一层去判断只会引入误判和无谓的耦合。

**这也是 ST 的姿势** —— ST 的 HAL DCMI 一个 cache 操作都没有，F7/H7 上靠 MPU 划非缓存区（AN4839），HAL 本身不检查。

### 10.2 调用方责任（非驱动逻辑，仅作提示）

若 framebuffer 放在可缓存的 write-back 区域，消费者读之前需要自行 invalidate，否则会读到 stale cache line（看到旧帧或新旧混合）。仓库里已有先例可参考：`bindings/honeygui/hardware_engine/PPE/AmebaG2/acc_ppe.c:511,908` 的 `DCache_CleanInvalidate()`。

最省事的做法是把 framebuffer 放非缓存区，一次维护都不用做。**但这是调用方的选择，驱动不参与、不检查、不报错。**

### 10.3 驱动自己的描述符环

描述符是 **CPU 写 → DMA 读**，若落在可缓存 write-back 内存里，CPU 写完可能还在 cache 没落到 SRAM，DMA 取到旧描述符会直接写飞。这是驱动自身的正确性问题，必须由驱动解决：

```c
/* rtl_dci_int.h */
#ifndef DCI_NONCACHE_SECTION
/* TODO(HAL): 指向真正的非缓存段。当前 rtl87x3g 的 section.h 只有放置段
 * （SRAM_DATA_SECTION / BUFFER_RAM_DATA_SECTION 等），没有 cacheability 属性段；
 * fmc_platform.h 的 FMC_MAINx_NON_CACHE_ADDR() 是地址别名法但只覆盖 flash(SPIC)，
 * 且这颗上 UNCACHEABLE_ADDR == ADDR（别名退化为恒等）、FMC_SUPPORT_MAIN0_CACHE=0。
 * SRAM/PSRAM 的非缓存机制待 SoC HAL 确认。 */
#define DCI_NONCACHE_SECTION
#endif
```

描述符只在 `Init` / `Reconfig` / `SetBuffers` 时写，不是每帧写，所以「非缓存段放置」和「写完 clean 一次」两种实现都可接受，取决于 HAL 提供什么。二者都收在 `rtl_dci_int.c`。

### 10.4 对 framebuffer 的唯一硬要求：4 字节对齐

不是为 cache，是 DMA 本身的要求（按 word 写）：

| 项 | 要求 | 说明 |
|---|---|---|
| `buf[]` 地址 | **必须 4 字节对齐** | DMA 按 word 写；`buf[]` 声明为 `uint32_t *` 即为自带提示 |
| `buf_bytes` | 4 的倍数 | 同上 |
| `buf_bytes` | ≥ `frame_bytes` | JPEG 下为容量上限 |

除此之外驱动不对 framebuffer 做任何假设。

---

## 11. 通知消费者：回调 + 轮询双通道

### 11.1 帧描述符

```c
typedef struct {
    uint8_t  *addr;      /* buffer 地址 */
    uint32_t  bytes;     /* 实际字节数，JPEG 变长时才有区别 */
    uint8_t   index;     /* 释放时回传 */
    uint32_t  seq;       /* 单调帧序号 */
    bool      truncated; /* true = 干净前缀但不完整（JPEG 越界 / 定长偏长偏短），§7.2.1 */
} DCI_frame_t;
```

`seq` 在**交付时**递增，所以它能揭示的是「**驱动交付了、但消费者没取到**」的帧 —— 消费者比较相邻两帧的 `seq` 差值即可发现：

| 会让 `seq` 跳号 | 说明 |
|---|---|
| `OVERWRITE` 模式下 buffer 被覆盖 | 交付过但消费者没赶上（§8.4） |
| POLL 模式下队列被顶掉 | 同上 |

**不会让 `seq` 跳号**（因为这些帧从未被交付）：门控暂停期间流过的帧、`FCRC` 硬件抽掉的帧、连续 JPEG 错过空白期跳掉的帧、OVR/resync 丢弃的帧。这些帧驱动自己也未必知道 —— 应用要掌握 sensor 的真实帧数，注册 `vsync_event`（§11.4）自行计数，`VSYNC` 是引脚跳变，与 `CAPTURE` 无关。

驱动侧已知的丢弃通过 `DCI_GetStats()` 的 `frames_dropped` / `suspend_count` / `resync_count` 上报。

### 11.2 两种通知模式，显式选择

```c
typedef enum {
    DCI_NOTIFY_CALLBACK = 0,   /* frame_ready 携带帧，不入队 */
    DCI_NOTIFY_POLL     = 1,   /* 入队，消费者用 GetFrame 取；frame_ready 不被调用 */
} DCI_NOTIFY_MODE_T;
```

显式选择而不是「两者都发」，避免消费者同时用两条路导致重复处理。

**POLL 模式下 `frame_ready` 完全不被调用**（即使非 NULL）—— 不作「有货提示」用。要唤醒等待的任务，消费者自己用 `WaitFrame()` 或在 `vsync_event` 里做，不要给同一件事留两个入口。`frame_error` / `vsync_event` / `line_event` 在两种模式下都照常调用。

### 11.3 轮询接口

```c
/* 状态查询 */
DCI_CAPTURE_STATE_T DCI_Capture_GetState(void);      /* IDLE/BUSY/SUSPENDED/ERROR */
bool                DCI_Capture_IsFrameReady(void);  /* 有无待取帧 */
uint8_t             DCI_Capture_GetReadyCount(void); /* 待取帧数 */

/* 取帧 */
DCI_ERROR DCI_Capture_GetFrame(DCI_frame_t *frame);  /* 非阻塞出队，无帧返回 NO_FRAME */
DCI_ERROR DCI_Capture_WaitFrame(DCI_frame_t *frame, uint32_t timeout_ms);  /* 阻塞轮询 */

/* 归还 */
void DCI_Capture_ReleaseBuffer(uint8_t index);
```

`WaitFrame` 用与 IDU 相同的 busy-wait 计数风格（`IDU_BUSY_WAIT`），保持 `rtl_common` 不依赖 OS；需要真正让出 CPU 的版本由应用层用 `GetFrame` + 自己的信号量实现。

#### 「ready 队列」是什么

只在 POLL 模式下存在，作用是**把「ISR 判定帧完成」和「消费者实际来取」这两个时刻解耦**。CALLBACK 模式下帧直接进回调，不经过它。

它记录的是**哪些 buffer 已完成但消费者还没取走、以及取走顺序**。两个关键性质：

**1. 入队即上锁。** `lock(bufN)` 发生在 ISR 交付时（入队那一刻），不是消费者 `GetFrame()` 时。否则「已完成但未被取走」的 buffer 处于未锁状态，DMA 转一圈回来就把它覆盖了。因此一个 buffer 的生命周期是三段：

```
free ──ISR 交付──► locked + 在队列里（已完成，未取）
                        │
                  GetFrame() 取走
                        ▼
                   locked + 已取（消费者处理中）
                        │
                 ReleaseBuffer()
                        ▼
                      free
```

`buf_lock_mask` 只区分 locked / free，不区分后两段；要看后两段用 `ready_count`（dump 里有）。对门控而言 locked 就是 locked，无需细分。

**2. 不需要真的实现成数组队列。** 环形 LLI 决定了 buffer 完成顺序**永远是固定轮转**：buf0 → buf1 → buf2 → buf0 …（§6.4）。所以「队列」退化成两个字节：

```c
uint8_t ready_head;    /* 下一个交给消费者的 buffer 下标 */
uint8_t ready_count;   /* 待取帧数，0 .. buf_count */
```

`GetFrame()` 取 `ready_head`、`head = (head + 1) % buf_count`、`count--`；ISR 交付时 `count++`。门控保证 `count` 永不超过 `buf_count`（满了就停采集，§8.2），所以不会溢出。

**resync 时 `ready_head` 不需要复位**（实现时修正）。只要交付时遵守「队列空则 `ready_head = 本次交付的 buffer`」，`ready_head` 就自我对齐：

- 队列空 → 下一次交付直接把 head 定到实际的 buffer，与 resync 回到 group 0 自动一致。
- 队列非空 → 里面的帧是 resync 之前交付的，它们的轮转次序仍然有效，硬把 head 归 0 反而会**打乱已入队帧的取出顺序**。

原先写「归 0」是把 DMA 侧的重启点和消费者侧的队列头当成了同一件事，实际是两条独立的游标。

#### 深度上限 = `buf_count`，所以不需要「取最新帧」接口

`buf_count ≤ DCI_MAX_BUF`（2），加上门控封顶，**待取帧最多 2 个，陈旧度最多落后 1 帧**。实际使用中受 framebuffer 空间限制，本来也不会积压更多。

因此不引入 `GetLatestFrame()` 之类的 LIFO 接口 —— 现有原语已经够用，想丢掉中间帧只取最新的，三行就够：

```c
while (DCI_Capture_GetReadyCount() > 1) {         /* 排空到只剩最新一帧 */
    DCI_frame_t stale;
    DCI_Capture_GetFrame(&stale);
    DCI_Capture_ReleaseBuffer(stale.index);
}
DCI_Capture_GetFrame(&latest);
```

**与背压的衔接**：POLL 模式下消费者不取帧，`ready_count` 涨到 `buf_count` → 所有 buffer 被锁 → `CAPTURE` 门控关闭。和 CALLBACK 模式同样安全降级，不会数据损坏。

### 11.4 回调集合与中断使能策略

```c
void (*frame_ready)(const DCI_frame_t *frame);       /* CALLBACK 模式必需 */
void (*frame_error)(DCI_ERROR err, uint32_t ris);    /* 建议提供，见 §14 错误码 */
void (*vsync_event)(void);                           /* 可选 */
void (*line_event)(void);                            /* 可选 */
```

原则：**没有消费者的中断一律不使能。** DCI 的 5 个中断源按此分成两类：

| 中断源 | 使能条件 | 理由 |
|---|---|---|
| `OVR` / `ERR` | **恒开** | 驱动自身需要它做 resync 和错误上报，不依赖用户回调 |
| `FRAME` | **恒开** | 帧边界的唯一权威；**不做动态开关**，理由见 §7.2 |
| `VSYNC` | `vsync_event != NULL` | 1 次/帧，开销可忽略。用途：测帧率、sensor 存活看门狗、触发外部逻辑（补光/同步） |
| `LINE` | `line_event != NULL` | **466 行 @60fps = 28k 次/秒**，无回调时开它纯属浪费 |

`LINE` 这条尤其重要 —— ST 的 `HAL_DCMI_Init` 无条件使能 `LINE | VSYNC | ERR | OVR`，在没有 `LineEventCallback` 的工程里等于每秒白进 2.8 万次中断。**我们不照抄这个默认值。**

两个可选回调都不带参数：硬件不提供行号，`rtl_common` 也没有时钟源（不依赖 OS），需要计数或时间戳由应用自己做。

#### 已删除的回调

`chunk_ready(buf_idx, line_start, line_count)` 曾用于帧内流水。§6.3 定下「装不下才加块」后常见情形是每帧 1 块，此时块完成即整帧数据到齐，它会与 `frame_ready` 在同一瞬间连续触发、携带同一份数据，没有独立信息量；只有 2~3 块时才有区别，而那只在接近 spec 上限时被强制发生、用户无法预知也无法依赖。**已删除。**

### 11.5 所有权协议

```
交付 ──► 消费者拥有 frame.index，驱动不再写它
   ...处理...
DCI_Capture_ReleaseBuffer(index) ──► 归还，驱动可以再写
```

配合 §8 的门控：**忘了释放的后果是采集暂停，不是数据损坏** —— 这是本设计最重要的性质，把隐蔽的数据竞争变成显式、可观测的停摆。

#### `lock` 的时机：交付时，不是取走时

`lock()` 发生在 **ISR 交付那一刻**（入队即上锁）。若改成 `GetFrame()` 时才锁，「已完成但消费者还没来取」的 buffer 处于未锁状态，DMA 转一圈就把它覆盖了。

buffer 的三段生命周期（`buf_lock_mask` 只区分前两态，第三段靠 `ready_count` 区分）：

```
free ──ISR 交付时 lock()──► locked + 在 ready 队列（已完成，未取）
                                  │ GetFrame()
                                  ▼
                           locked + 已取（消费者处理中）
                                  │ ReleaseBuffer()
                                  ▼
                                free
```

#### 上下文：`ReleaseBuffer()` / `GetFrame()` 两种上下文皆可

**明确允许在 `frame_ready` 回调（即 ISR 上下文）里同步调用 `ReleaseBuffer()`** —— 这正是「就地把 buffer 挂给 PPE/LCDC 启链、延迟最低」那条用法所需。禁止它会强迫消费者改成"投信号量 → 任务 → 处理 → 释放"，每帧多一次上下文切换，且**强制消费者侧依赖 OS**。

这带来一条**不可逆的实现约束**：

> §11.6 的 `rtl_dci_lock()` **必须是关中断或等价手段，不能是信号量/互斥量** —— ISR 里 take 信号量会阻塞或断言失败，自旋锁则会在 ISR 打断持锁线程时死锁。

其余影响都可控：临界区内不得阻塞（本来就该如此）；`ReleaseBuffer` 只做清位 + 可能写 `CR.CAPTURE = 1`，而**置位不需要回读轮询**（只有清位需要，§8.2），所以没有等待；嵌套 `ISR → frame_ready → ReleaseBuffer → 临界区` 在关中断下天然安全。代价是测试面翻倍 —— 每条 lock/release 路径要在两种上下文各测一遍。

**例外：`DCI_Capture_WaitFrame()` 不可在 ISR 调用**，它是 busy-wait 自旋。

#### 两条保证

- **可乱序释放。** 门控检查是「下一个要写的 buffer 是否被锁」，逐 buffer 判断，与释放顺序无关。
- **所有权只在帧级转移。** 块完成不转移所有权（`chunk_ready` 已删除，§11.4）；将来若重新引入块级通知，也不应赋予它所有权语义。

#### `Abort()`：不提供作废机制，靠契约

**驱动不做 buffer 作废检测**（不引入 epoch 之类的失效标记）。契约是：

> **`Abort()` 返回后，此前交付的所有帧立即失效，应用不得再访问 `frame.addr`。** 驱动会清空 `buf_lock_mask`，下一次 `Start()` 可能立刻覆写那些内存。

与 `Reconfig()` 形成明确的两条路径：

| | 有 buffer 被持有时 | 用途 |
|---|---|---|
| `Reconfig()` / `SetBuffers()` | 返回 `DCI_ERROR_BUSY`，**不动硬件** | 安全路径，消费者先归还 |
| `Abort()` | **强行回收** | 显式逃生口 —— 消费者卡死或应用要立即停时用，代价是接受丢失手上那帧 |

`Abort()` 之后调 `ReleaseBuffer()` 是无害的空操作（对应位已被清）。

ST 用的是 weak 回调 + 新版的运行时注册（`HAL_DCMI_RegisterCallback`），且**没有任何轮询接口**（回调不带参数，只有 `hdcmi`）。我们用配置结构里的函数指针（等价于运行时注册且更简单，weak 覆盖在多模块工程里容易撞），并补上 ST 没有的轮询通道。

### 11.6 共享状态的并发保护

三处状态被 ISR 和线程双方读写，**必须有临界区**，否则是实打实的数据竞争：

| 状态 | ISR 侧 | 线程侧 |
|---|---|---|
| `buf_lock_mask` | 交付时 set 某位 | `ReleaseBuffer()` clear 某位 |
| `ready_head` / `ready_count` | `count++` | `GetFrame()` 改 head、`count--` |
| `state`（BUSY/SUSPENDED/ERROR） | 门控时置 SUSPENDED、出错置 ERROR | `Release`/`Resume` 置 BUSY |

即使只是一个字节的位掩码，「ISR set 位 M」与「线程 clear 位 N」之间仍有读-改-写窗口，不能靠「位不同所以安全」蒙过去。

照 IDU 的模式做成 SoC 层钩子，让 `rtl_common` 只调用不实现，从而保持不依赖 OS（`rtl_idu_int.c:18-27` 是现成范例）：

```c
/* rtl_dci_int.h */
uint32_t rtl_dci_lock(void);
void     rtl_dci_unlock(uint32_t s);
```

**实现约束：必须是关中断或等价手段，不能用信号量/互斥量。** 因为 §11.5 明确允许在 `frame_ready` 回调（ISR 上下文）里同步调 `ReleaseBuffer()` —— ISR 里 take 信号量会阻塞或断言失败，自旋锁则会在 ISR 打断持锁线程时死锁。IDU 的 `os_lock()/os_unlock()` 正是关中断这一类。**这条不可逆，不要事后"优化"成 mutex。**

临界区里只做几个字的读改写，不调回调、不打印、不等待。

---

## 12. 运行时重配置

### 12.1 硬件前提

寄存器表对 `CR.ENABLE` 的说明是硬约束：

> Program configuration registers before enabling this bit.

**配置寄存器必须在 `ENABLE=0` 时写。** 所以不存在「热改分辨率/极性」的可能，必须走停—改—启序列。设计上的任务是把这个序列做**安全**和**优雅**（停在帧边界，不产生撕裂帧和 OVR）。

例外：**`IER` 可以在采集中直接写。** 证据是 ST 的 HAL 在采集过程中动态开关 `FRAME` 中断（`:1078` 开、`:635` 关）。我们自己不做动态开关（§7.2），但 `Reconfig` 期间仍要改 `IER`，所以这条热写属性是需要的。`ICR` 是 write-1-clear，同样是热的。

### 12.2 各类改动的代价

| 改动 | 需停 CAPTURE | 需 `ENABLE=0` | 需重建描述符 |
|---|---|---|---|
| **`on_busy` / `notify_mode` / 回调指针** | **否** | **否** | **否** —— 纯软件字段，见 §12.3 末 |
| `IER` 中断使能 | 否 | 否 | 否 |
| `ICR` 清中断 | 否 | 否 | 否 |
| `FCRC` 帧率抽取 | 是 | 是 | 否 |
| `ESCR` / `ESUR` 同步码 | 是 | 是 | 否 |
| 极性（PCK/HS/VS）| 是 | 是 | 否 |
| `EDM` / `ESS` / `JPEG` | 是 | 是 | 是（帧长变，JPEG 还改拓扑） |
| crop 窗口 / `CWSIZE` / `CropCmd` | 是 | 是 | 是（帧长变） |
| sensor 几何声明（§12.6-D1） | 是 | 是 | 是（帧长变） |
| buffer 地址（尺寸不变）| 是 | 否 | 只改各描述符 DAR |
| `buf_count` | 是 | 否 | 是 |
| DMA 通道号 / 优先级 | 是 | 否 | 是（换通道） |
| **capture mode（SNAPSHOT ↔ CONTINUOUS）** | — | — | **不支持运行时变更**，见下 |

#### capture mode 不支持运行时变更

SNAPSHOT 与 CONTINUOUS 的描述符拓扑不同（N 条 `LLP=0` 链 vs 闭环，§6.4），换 mode 等于重建一切。而**实际场景里不存在"采集途中从连拍切到流式"的需求** —— 用途一开始就定了。

所以：**`Start(mode)` 的 mode 在首次调用时确定拓扑，后续 `Start()` 必须传相同值**，不同则返回 `DCI_ERROR_INVALID_PARAM`。真要换 mode，重新 `DCI_Capture_Init()`。

### 12.3 统一流程

```c
DCI_ERROR DCI_Capture_Reconfig(const DCI_InitTypeDef *init,
                               const DCI_DMA_config *dma);   /* 任一可为 NULL = 该部分不变 */
```

```
1. 全量校验新参数（§15 全部检查项）
   └─ 任一失败 → 立即返回错误，硬件一个字都不碰，正在跑的采集不受影响
2. 若有 buffer 仍被消费者持有 → 返回 DCI_ERROR_BUSY
3. 记录当前是否在采集（决定第 8 步要不要自动重启）
4. 优雅停：清 CAPTURE + 回读轮询到 0        （停在帧边界）
5. GDMA_Cmd(ch, DISABLE)                    （safe suspend）
6. DCI_Cmd(DISABLE)                         （ENABLE=0，允许改配置；同时复位 FIFO）
7. 重算 line_words / chunks_per_frame / desc_count，按需重建描述符环
   DCI_Init(新配置)；DCI_Cmd(ENABLE)
8. 若第 3 步在跑：DMA re-arm 到 desc[0]，CAPTURE=1
```

**两条关键设计决定**：

- **先校验后破坏**（第 1 步）。配置错误不应该把一个正在正常工作的采集搞停。这是 `Reconfig` 与「先 Stop 再 Init」的本质区别，也是提供这个 API 的主要理由。
- **持锁则拒绝**（第 2 步）。重配置会让消费者手里的 buffer 语义失效（尺寸/格式可能变）。返回 `DCI_ERROR_BUSY` 让消费者先归还，是确定性的做法。消费者可用 `GetReadyCount()` 和 dump 里的 `buf_lock_mask` 自查。需要强行回收时用 `DCI_Capture_Abort()` —— **驱动不提供作废检测，靠契约**：`Abort()` 返回后此前交付的帧立即失效，应用不得再访问，详见 §11.5。

#### 纯软件字段跳过硬件序列

`on_busy`、`notify_mode`、四个回调指针都不映射到任何寄存器，也不影响 `frame_bytes` 或描述符拓扑。`Reconfig` 应先比对新旧配置，**若只有这些字段变化，直接改内部状态后返回，跳过第 4~8 步** —— 否则会为一次纯软件改动白停一帧采集。

比对在临界区内做（§11.6），改完即生效；正在处理中的那一帧仍按旧策略走完。

### 12.4 快路径

只换 buffer 地址（分辨率、格式、块划分都不变）是最常见的运行时改动（例如上层轮换内存池），不必走完整序列：

```c
DCI_ERROR DCI_Capture_SetBuffers(uint32_t *const buf[], uint8_t count);
```

```
1. 校验（对齐、count 与 buf_count 一致、无 buffer 被持有）
2. 优雅停 + GDMA suspend           /* ENABLE 保持 1，不重配 DCI */
3. 只改描述符环各项的 DAR
4. re-arm desc[0] + CAPTURE=1
```

> 第 1 步要求「无 buffer 被持有」，所以 **`SetBuffers()` 不能用来做快照轮转** —— 应用正拿着 buf0 处理时恰好调不了它，会死锁。快照要边处理边采集，用 `buf_count = 2` + 驱动内部轮转（§6.4）。

其余单项改动（`FCRC`、同步码、极性）提供便捷封装，内部都走 §12.3 的核心序列：

```c
DCI_ERROR DCI_SetCaptureRate(DCI_FCRC_T rate);
DCI_ERROR DCI_ConfigSyncCode(const DCI_SyncCode_t *code, const DCI_SyncUnmask_t *unmask);
DCI_ERROR DCI_ConfigCrop(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
DCI_ERROR DCI_CropCmd(FunctionalState state);   /* 开/关 crop 改变 frame_bytes，会失败 */
```

### 12.5 状态机

```
        ┌────────────────── Abort ──────────────────┐
        ▼                                            │
    ┌──────┐   Start    ┌──────┐   门控/Suspend  ┌───────────┐
    │ IDLE │ ─────────► │ BUSY │ ◄─────────────► │ SUSPENDED │
    └──────┘            └──────┘   Release/Resume└───────────┘
        ▲                   │
        │  Stop             │ OVR / ERR
        └───────────────────┤
                            ▼
                        ┌───────┐
                        │ ERROR │ ── resync ──► BUSY
                        └───────┘
```

`Reconfig` 在 IDLE / BUSY / SUSPENDED / ERROR 均可调用（它本身就是一次完整重初始化）。

### 12.6 运行时参数修改流程

上面几节是机制，本节是调用方视角的三条流程。**分界线不是「改的是 camera 还是 DCI」，而是「改动是否影响 `frame_bytes`」** —— 因为 `frame_bytes` 决定描述符环（§6.2）。

```
                   改动影响 frame_bytes 吗？
                          │
        ┌─────────────────┴─────────────────┐
        否                                  是
        │                                   │
   流程 A：驱动完全不参与              需要重建描述符环
   （曝光/增益/白平衡/gamma）                │
                            ┌───────────────┴───────────────┐
                       改的是 sensor                    只改 DCI
                            │                               │
                      流程 B：有顺序约束              流程 C：单次调用
                      （分辨率/格式/位深）             （crop 窗口 / CropCmd）
```

#### 流程 A —— 不影响帧几何的 camera 参数

曝光、增益、白平衡、gamma 这类，sensor 输出的分辨率/格式/速率都不变。

```c
/* 驱动完全不参与，采集不用停 */
sensor_write_reg(SENSOR_REG_EXPOSURE, val);     /* I2C / SCCB */
```

DCI 侧零操作 —— 它只管数 word，不关心像素内容。

唯一注意点是**过渡帧**：多数 sensor 在下一个帧边界才生效，切换瞬间可能有一两帧新旧参数混合。消费者若在意，用 `frame.seq` 标记改动时刻并丢弃之后 N 帧。N 是 sensor 相关的，驱动推不出来，属应用层职责。

#### 流程 B —— 影响帧几何的 camera 参数

分辨率、格式（RGB565 ↔ YCbCr422 ↔ JPEG）、位深切换。**顺序是关键**：

```c
/* 1. 先停 DCI —— 必须在动 sensor 之前 */
DCI_Capture_Stop();                     /* 清 CAPTURE + 回读轮询到 0，再 ENABLE=0 */

/* 2. 改 sensor */
sensor_set_mode(SENSOR_MODE_QVGA);      /* I2C */

/* 3. 等 sensor 稳定 —— sensor 驱动的责任 */
sensor_wait_stable();

/* 4. 重配 DCI（几何变了 → 重建描述符环） */
DCI_Capture_Reconfig(&new_init, NULL);  /* dma 部分不变则传 NULL */

/* 5. 重启 */
DCI_Capture_Start(DCI_MODE_CONTINUOUS);
```

**为什么必须先停 DCI。** DCI 完全不知道 sensor 在干什么，它只按 `BLOCK_TS` 数 word。若 sensor 先改小而 DCI 还按旧几何的环在跑，帧尾 word 数对不上 → 块完不成 → 下一帧继续填同一块 → 失步（§7.1 第 2 种情形）；若改大则 FIFO 来不及 → OVR。**任何错配都是失步。**

**为什么 `Stop()` 而不是 `Suspend()`。** `Suspend()` 只清 `CAPTURE`、保留 `ENABLE=1`，而 sensor 模式切换过程中 PIXCLK 可能停止或抖动，`VSYNC` 中断会乱跳。`Stop()` 一路清到 `ENABLE=0`（同 ST 的 `HAL_DCMI_Stop`：清 CAPTURE → 轮询 → `__HAL_DCMI_DISABLE` → abort DMA），把 DCI 彻底隔离在外。而且几何变了旧环无论如何都要重建，`Suspend` 保住的状态没有价值。

**为什么第 5 步要显式 `Start()`。** `Reconfig` 只在「调用前正在跑」时才自动重启（§12.3 第 3/8 步），而这里已经 `Stop()` 过，所以它不会自动起 —— 这个交互是有意为之，正好把重启时机交给调用方，让它能在第 3 步等够时间。

#### 流程 C —— 只改 DCI crop

sensor 不动，只改 `CWSTRT`/`CWSIZE`。一次调用搞定：

```c
DCI_ERROR err = DCI_ConfigCrop(x0, y0, w, h);   /* 内部走 §12.3 核心序列 */
```

内部展开：

```
1. 全量校验：w/h ≤ 2048、capcnt 4 对齐、crop 不超出 sensor 输出、
             buf_bytes ≥ 新 frame_bytes、desc_count ≤ DCI_MAX_DESC
   └─ 任一失败 → 直接返回，硬件不碰，正在跑的采集不受影响
2. 有 buffer 被消费者持有 → 返回 BUSY
3. 记录当前是否在跑
4. 清 CAPTURE + 回读轮询到 0            ← 停在帧边界，不产生撕裂帧
5. GDMA_Cmd(ch, DISABLE)
6. DCI_Cmd(DISABLE)                     ← ENABLE=0 才允许写配置寄存器（§12.1）
7. 写 CWSTRT/CWSIZE；重算 line_words / chunks_per_frame；重建描述符环
8. DCI_Cmd(ENABLE)
9. 若第 3 步在跑 → re-arm desc[0] + CAPTURE=1
```

与流程 B 的区别：**不需要等待稳定**，重启后第一帧即有效，因为 sensor 输出一直没变。

`DCI_CropCmd(ENABLE/DISABLE)` 走同一条序列 —— 开/关 crop 本身就改变 `frame_bytes`（关掉即变成 sensor 全帧），不是一个简单的位写。

#### 本节依赖的待补项

| # | 缺漏 | 状态 |
|---|---|---|
| ~~D1~~ | ~~`DCI_InitTypeDef` 未定义 sensor 几何字段~~ | **已修**：`sensor_pclk_per_line` / `sensor_lines` 已在 `DCI_InitTypeDef`（§14），公式在 §6.2 |
| ~~D2~~ | ~~`frame_bytes` 完整公式未写，连带两条缺失校验~~ | **已修**：公式见 §6.2（含 `EDM`/`BSM`/`LSM`/`OELS`），`bsm != 0 → edm == 0` 与 crop 越界两条校验见 §15 |
| ~~D3~~ | ~~`DCI_CropCmd()` 签名~~ | **已修**：§12.4 / §14 均已改为 `DCI_ERROR` |

---

## 13. 调试接口：Reg Dump

### 13.1 三条设计原则

**1. 绝不读 `DR`。** 它是 FIFO 出口，读一次弹一个 word，dump 会直接破坏正在进行的采集。输出里显式标注跳过，避免以后有人「顺手补全」。

**2. 采集与打印分离。** OVR 发生在 ISR，那一刻的寄存器状态才有诊断价值，但像素速率下不能在 ISR 里打印。

**3. 不止 DCI 寄存器。** 调试痛点大半在 DMA 侧和软件状态。

### 13.2 接口

```c
void DCI_RegSnapshot(DCI_snapshot_t *snap);          /* ISR 安全，只搬寄存器不打印 */
void DCI_DumpSnapshot(const DCI_snapshot_t *snap);   /* 线程上下文打印 */
void DCI_RegDump(void);                              /* 便捷入口 = Snapshot + Dump */
void DCI_RegMonitor(void);                           /* 只打印相对上次变化的字段 */
void DCI_GetStats(DCI_stats_t *stats);               /* 纯计数器，无打印 */

DCI_ERROR DCI_RegTest(DCI_regtest_t *report);        /* 寄存器读写自检，见 §13.6 */
DCI_ERROR DCI_RegTestDump(void);                     /* = RegTest + 打印报告 */
```

### 13.3 快照内容

```c
typedef struct {
    /* --- DCI 寄存器（排除 DR） --- */
    uint32_t cr, sr, ris, ier, mis, icr;
    uint32_t escr, esur, cwstrt, cwsize;

    /* --- GDMA 通道 --- */
    uint32_t dma_sar,  dma_curr_sar;
    uint32_t dma_dar,  dma_curr_dar;
    uint32_t dma_llp;
    uint32_t dma_ctl_low, dma_ctl_high;      /* ctl_high 读回 = TRANS_DATA_CNT */
    uint32_t dma_cfg_low, dma_cfg_high;

    /* --- GDMA 全局 --- */
    uint32_t dma_ch_en;
    uint32_t dma_status_tfr, dma_status_block, dma_status_err;

    /* --- 环形 LLI --- */
    uint8_t  desc_count;
    struct { uint32_t sar, dar, llp, ctl_low, ctl_high; } desc[DCI_MAX_DESC];

    /* --- 驱动软件状态 --- */
    uint8_t  state;              /* IDLE / BUSY / SUSPENDED / ERROR */
    uint8_t  xfer_mode;          /* FIXED_LEN / VAR_LEN */
    uint8_t  notify_mode;        /* CALLBACK / POLL */
    uint8_t  buf_count;
    uint8_t  chunks_per_frame;
    uint16_t lines_per_chunk;
    uint8_t  line_aligned;       /* 0 = 退化为 word 对齐分块 */
    uint8_t  cur_desc_idx;
    uint8_t  chunk_idx;          /* 组内块进度 */
    uint8_t  evt;                /* 会合位：bit0=FRAME 已到, bit1=组末块已到，§7.2 */
    uint8_t  buf_lock_mask;      /* 哪些 buffer 被消费者持有 */
    uint8_t  ready_count;        /* POLL 模式待取帧数 */
    DCI_stats_t stats;
} DCI_snapshot_t;
```

```c
typedef struct {
    uint32_t frames_done;      /* 成功交付 */
    uint32_t frames_dropped;   /* 仅驱动明确知道被丢弃的：resync 废帧、OVR 废帧。
                                * 门控期间流过的帧和 FCRC 抽掉的帧驱动观测不到，§11.1 */
    uint32_t chunks_done;
    uint32_t ovr_count;        /* FIFO 溢出 */
    uint32_t err_count;        /* 内嵌码顺序错误 */
    uint32_t resync_count;
    uint32_t suspend_count;    /* 因消费者迟到而暂停的次数 */
    uint32_t buf_overflow;     /* JPEG 超出 buffer 容量 */
    uint32_t seq;              /* 当前帧序号 */
} DCI_stats_t;
```

`suspend_count` / `frames_dropped` 是判断消费者跟不跟得上的直接指标，比看寄存器有用。

### 13.4 输出格式

原始 hex 之外必须解码字段。`CWSIZE`/`CWSTRT` 这类 N-1 编码的同时打印原值和实际值：

```
--- DCI @0x40643000  state=BUSY  mode=CONT  buf=2/2  chunks=2/frame(233 lines, aligned) ---
CR     0x00004081  EN=1 CAP=1 CM=0(cont)  CROP=0 JPEG=0 ESS=0
                   EDM=0(8b) FCRC=0(all)  PCK=0(fall) HS=0(low) VS=0(low)
                   BSM=0 OEBS=0 LSM=0 OELS=0
SR     0x00000004  FNE=1 VSYNC=0 HSYNC=0
RIS    0x00000000  FRAME=0 OVR=0 ERR=0 VSYNC=0 LINE=0
IER    0x0000000F  FRAME=1 OVR=1 ERR=1 VSYNC=1 LINE=0     <- FRAME 恒开；LINE 无回调故不开
MIS    0x00000000  -
CWSTRT 0x00000000  VST=0(line1) HOFF=0
CWSIZE 0x01D100EF  VLINE=0x1D1(466 lines) CAPCNT=0xEF(240 pclk)
ESCR   0x00000000  FEC=00 LEC=00 LSC=00 FSC=00
ESUR   0x00000000  FEU=00 LEU=00 LSU=00 FSU=00
DR     (skipped - reading pops FIFO)

--- GDMA ch0 ---
SAR    0x40643028  CURR_SAR 0x40643028        <- 应恒等于 &DCI->DR（base + 0x28）
DAR    0x00820000  CURR_DAR 0x008386A0        <- 帧内进度
LLP    0x00810014
CTL_LO 0x00305001  INT_EN=1 DSTW=2(word) SRCW=2(word) DINC=0(inc) SINC=2(fix)
                   DMSIZE=2(8) SMSIZE=0(1) TT_FC=2(per2mem) LLP_DST=1 LLP_SRC=0
CTL_HI 0x000061A8  TRANS_DATA_CNT=25000
CFG_LO 0x00000000  HS_SEL_DST=0(hw) HS_SEL_SRC=0(hw) RELOAD_SRC=0 RELOAD_DST=0
CFG_HI 0x00028000  SRC_PER=0x30 DEST_PER=0            <- 0x30 = 48，DCI on AP DMA1
CH_EN  0x00000101  StatusBlock 0x00000000  StatusErr 0x00000000

--- LLI ring (4 desc, 2 grp x 2, CONTINUOUS 闭环) ---
grp0 [ 0] DAR 0x00820000 LLP 0x00810014 CTL_HI 54289
grp0 [ 1] DAR 0x0083503C LLP 0x00810028 CTL_HI 54289
grp1 [ 2] DAR 0x008A0000 LLP 0x0081003C CTL_HI 54289
grp1 [ 3] DAR 0x008B503C LLP 0x00810000 CTL_HI 54289   <- 环闭合回 [0]
                                        ΣCTL_HI = 217,156 = frame_words × 2 ✓

--- stats ---
done 1204  dropped 3  chunks 1204  ovr 1  err 0  resync 1  suspend 2
buf_overflow 0  seq 1207  cur_desc 0  chunk_idx 0  evt 0x00  lock_mask 0x02  ready 0
```

刻意放进去的交叉校验点：

| 检查项 | 正常应满足 | 不满足说明 |
|---|---|---|
| `CURR_SAR == &DCI->DR` | 恒等 | SAR 配错或 `SINC` 不是 Fix |
| `SRC_PER` | **`0x30` = 48**（DCI on AP DMA1） | 其他值 = 握手配错；注意该字段是 4+3 位拼的（`[10:7]` + bit15/17/19），只看低 4 位会把 48 读成 0 |
| `HS_SEL_SRC == 0` | 硬件握手 | 为 1 是软件握手，复位默认值没清 |
| `SMSIZE == 0` | `Msize_1` | 非 0 会欠载（spec §2.3 逐 word 请求）|
| `DMSIZE` 与通道深度匹配 | ch0-5 → 2(8)，ch6-15 → 1(4) | 为 0(1) 说明退化成单字写，丢了 AHB 突发（§5）|
| `LLP` 拓扑 | CONTINUOUS：末描述符回到 `desc[0]`；SNAPSHOT / JPEG：每组末 `LLP = 0` | 与 mode 不符 = 描述符建错（§6.4） |
| 各 `CTL_HI` 之和 | 定长：== `frame_words × buf_count` | 分块算错 |
| `chunk_idx` ≤ `chunks_per_frame` | 是 | 超出 = 失步 |
| `evt`（仅 CONTINUOUS 定长） | 稳态下多为 0 | 长期停在 `0x01`/`0x02` = 会合缺一半，采集已挂（§7.2） |
| `dropped` / `suspend` 增长 | 稳定 | 持续增长 = 消费者跟不上，考虑 `FCRC` 降帧或 `buf_count = 2` |
| `buf_overflow` | 0 | JPEG buffer 太小 |

### 13.5 编译期开关与日志后端

整块 dump 由 `DCI_DEBUG_REG_DUMP` 门控，关闭时零代码零 RAM（快照结构体也不占）：

```c
#ifndef DCI_DEBUG_REG_DUMP
#define DCI_DEBUG_REG_DUMP    0
#endif
```

日志宏可覆盖，默认走仓库约定的 `DBG_DIRECT`（`trace.h`，JPU/IDU 同）：

```c
#ifndef DCI_LOG
#define DCI_LOG(...)    DBG_DIRECT(__VA_ARGS__)
#endif
```

**例外：`DCI_RegTest()` 不受这个开关门控。** 它是 bring-up 的第一道工序，必须在 release 配置下也能调；只有 `DCI_RegTestDump()` 的打印部分受开关控制（关闭时它仍返回判定结果，只是不打印）。

### 13.6 寄存器读写自检

```c
DCI_ERROR DCI_RegTest(DCI_regtest_t *report);   /* report 可为 NULL */
DCI_ERROR DCI_RegTestDump(void);                /* = RegTest + 打印报告 */
```

**存在的理由：把「时钟没开」和「基址写错」这两个错误提前到第一条指令就暴露。** 这两者的症状都是「采集不出数据」，而那个症状有几十种可能的原因；等到接了 sensor、配好 DMA 再去查，会浪费大量时间。不需要 camera、不需要 DMA、不需要时钟以外的任何前提。

#### 测什么

| 检查 | 手段 | 能查出 |
|---|---|---|
| **活性**（先做） | 往 `CR` 写 0 与写 `rw_mask`，比较两次读回 | **时钟未开 / 基址错 / 总线死** —— 见下方"为什么不判具体值" |
| 位可写性 | 每个可写位 walking-1，再加 all-1 / `0x5555…` / `0xAAAA…` | 某位恒 0（`stuck_0`）、某位恒 1 或位间串扰（`stuck_1`）。walking-1 定位单个坏位，交替图案抓相邻位短路 |
| 保留位 | 写 1 后读回必须为 0 | 位域定义与实际硬件不符 |
| **地址不混叠** | 每个寄存器写各不相同的值，再全部读回 | 偏移解码错、或整块指向了错误的外设 —— 混叠时所有寄存器读出同一个值 |
| `ICR` write-1-only | 写 `ICR = ALL` 后 `RIS` 仍应为 0（无像素活动） | `RIS` 被误实现成可写、或 `ICR` 语义不符 |
| `MIS = RIS & IER` | 置 `IER = ALL`，`MIS` 仍应为 0 | `MIS` 恒 1 之类的硬件问题 |
| `CAPCNT[1:0]` | 写 1 看读回 | **顺带定论 §17 项 11** —— 读回 0 说明硬件强制清位，读回 1 说明是纯 N-1 编码 |

`DR` 全程不碰（读一次弹一个 word，§13.1）。测试结束把所有寄存器恢复复位值、清掉 pending 中断，并把驱动状态机置回 `RESET`；若调用时正在采集，会先 `Abort()` 拆掉，所以**不可在采集中调**。

#### 两处刻意排除

| 排除项 | 理由 |
|---|---|
| **`CR.capture`（bit 0）** | 寄存器表标 **R/WAC** —— 硬件会自己清（快照模式下帧尾自清，§5.2）。一个可能合法地拒绝保持的位无法做图案测试，**测了会在 bring-up 第一步就误报 stuck-0**，比不覆盖更糟。它由每一条 capture 用例充分覆盖 |
| **`DR`** | 读一次弹一个 word |

#### 为什么活性判据是「读回是否变化」而不是「读回是否为某个值」

实现时先写成了「`seen_bits` 为全 0 或全 1 即判死」，**这是错的**：`ESCR`/`ESUR` 是完整的 32 位可写寄存器，往里写 `0xFFFFFFFF` 再读回来本来就是全 1，会被误判成死块。正确的判据是**同一个寄存器写不同值时读回是否跟着变** —— 时钟没开的块无论写什么都返回同一个常量。用 `CR` 做探针，因为它的 `rw_mask` 里既有该置 1 的位也有该保持 0 的保留位，写 0 与写 `rw_mask` 在任何正常块上必然读出两个不同的值。

#### 报告结构

```c
typedef struct {
    uint32_t base, probe_lo, probe_hi;   /* probe_lo == probe_hi 即判死 */
    uint8_t  checked, failed;
    bool     responded, varied, distinct_ok, w1c_ok, mis_ok;
    bool     capcnt_low_forced;          /* 信息项，不算失败 */
    DCI_regtest_item_t item[10];         /* 每个寄存器一项，含 stuck_0/stuck_1 */
} DCI_regtest_t;
```

返回三种结论：`DCI_SUCCESS` / `DCI_ERROR_NO_RESPONSE`（块像是死的，其余结果全部无意义故不再检查）/ `DCI_ERROR_REG_TEST`（块有响应但某些位行为不对）。**分成两个错误码而不是一个**，因为调用方要做的事完全不同：前者查时钟和基址，后者查位域定义或报硬件问题。

正常输出：

```
--- DCI register test @0x40643000 ---
  CR     +0x00  rw_mask 0x001F4FFE  OK      <- bit0 (capture, R/WAC) 不在掩码内
  SR     +0x04  read-only, no read-back to test
  RIS    +0x08  read-only, no read-back to test
  IER    +0x0C  rw_mask 0x0000001F  OK
  MIS    +0x10  read-only, no read-back to test
  ICR    +0x14  write-1-only, no read-back to test
  ESCR   +0x18  rw_mask 0xFFFFFFFF  OK
  ESUR   +0x1C  rw_mask 0xFFFFFFFF  OK
  CWSTRT +0x20  rw_mask 0x1FFF3FFF  OK
  CWSIZE +0x24  rw_mask 0x3FFF3FFF  OK
  distinct=1 varied=1 w1c=1 mis=1   checked=6 failed=0
  CAPCNT[1:0] writable -> encoding is plain N-1
  verdict: PASS
```

失败时的样子（时钟未开）：

```
--- DCI register test @0x40643000 ---
  NO RESPONSE: CR read 0x00000000 after writing 0, 0x00000000 after writing 0x001F4FFE
  check: DCI clock gated? base address wrong? bus/power down?
```

---

## 14. API 汇总

### 错误码

按「调用方能做什么」分三组，而不是按发生位置分：

```c
typedef enum
{
    DCI_SUCCESS = 0,

    /* ---- 配置类：调用方需修正参数，改了才可能成功 ---- */
    DCI_ERROR_INVALID_PARAM,      /* 通用非法参数（空指针、越界枚举等） */
    DCI_ERROR_SIZE,               /* 分辨率超 2048×2048，或帧超 DCI_MAX_CHUNK 块 */
    DCI_ERROR_ALIGN,              /* buf 地址或 buf_bytes 非 4 字节对齐 */
    DCI_ERROR_BUF_TOO_SMALL,      /* buf_bytes < frame_bytes（定长格式） */
    DCI_ERROR_MODE_CONFLICT,      /* 互斥组合：jpeg+crop / jpeg+ess / ess+edm!=0 */
    DCI_ERROR_NO_CALLBACK,        /* notify_mode=CALLBACK 但 frame_ready 为 NULL */
    DCI_ERROR_DMA_CHANNEL,        /* 通道号非法或申请失败 */

    /* ---- 状态类：参数没错，是时序不对，稍后重试或先换状态 ---- */
    DCI_ERROR_NOT_INIT,           /* 未 Capture_Init 就调 Start/Reconfig */
    DCI_ERROR_STATE,              /* 当前状态不允许此操作（如 BUSY 时再 Start） */
    DCI_ERROR_BUSY,               /* 目标 buffer 仍被消费者持有，需先 Release */
    DCI_ERROR_NO_FRAME,           /* GetFrame 时无待取帧 */
    DCI_ERROR_TIMEOUT,            /* 回读轮询超时：CAPTURE 未清或 FIFO 未排空 */

    /* ---- 运行时类：只经 frame_error 回调上报，不作为函数返回值 ---- */
    DCI_ERROR_OVERRUN,            /* IT_OVR：FIFO 溢出，整帧丢失 */
    DCI_ERROR_SYNC,               /* IT_ERR：内嵌同步码顺序错误 */
    DCI_ERROR_BUF_OVERFLOW,       /* JPEG 超出 buffer 容量（SNAPSHOT），§9.2 */
    DCI_ERROR_FRAME_LONG,         /* 定长：实际输出超出声明几何（SNAPSHOT），§7.2.1 */
    DCI_ERROR_FRAME_SHORT,        /* 定长：实际输出小于声明几何（SNAPSHOT），§7.2.1 */
    DCI_ERROR_DESYNC,             /* 块/帧计数失配，已 resync（CONTINUOUS），§7.2 */

    /* ---- bring-up 类：仅 DCI_RegTest() 返回 ---- */
    DCI_ERROR_NO_RESPONSE,        /* 寄存器块无响应：时钟未开 / 基址错 / 总线死，§13.6 */
    DCI_ERROR_REG_TEST,           /* 块有响应但某些位行为不对，§13.6 */
} DCI_ERROR;
```

#### `DCI_Capture_Start()` 的失败码

| 情形 | 返回 | 调用方该做什么 |
|---|---|---|
| 未 `Capture_Init` | `NOT_INIT` | 先 Init |
| 当前 BUSY / SUSPENDED | `STATE` | 先 `Stop()` 或 `Abort()` |
| SNAPSHOT 下轮转的目标 buffer 仍被持有 | `BUSY` | 先 `ReleaseBuffer()`，或稍后重试 |
| 清 `CAPTURE` 回读轮询超时 | `TIMEOUT` | 硬件异常，走 `Abort()` + reg dump |

从 **ERROR 状态调 `Start()` 是允许的**（清错误后重新装载）—— 错误早已通过 `frame_error` 回调通知过，再要求一次 `Abort()` 只是无意义的仪式。SNAPSHOT 本来每次都从零重装通道，没有残留状态需要清理。

#### `frame_error` 回调签名要改

原签名 `(uint32_t ris, uint32_t dropped)` **表达不了驱动侧检测的错误** —— `BUF_OVERFLOW` 和 `DESYNC` 不是 DCI 的 `RIS` 位，前者来自 `GDMA_INT_Transfer`、后者来自软件计数比对。改成：

```c
void (*frame_error)(DCI_ERROR err, uint32_t ris);
```

`err` 是驱动的分类结论，`ris` 是当时的 DCI 原始状态供诊断（错误不源自 DCI 时为 0）。丢帧数不再挤进签名 —— 用 `DCI_GetStats()` 取（`seq` 只揭示"交付了但未取到"，见 §11.1）。

### 外设层（只碰 DCI 寄存器）

```c
void     DCI_StructInit(DCI_InitTypeDef *init);
void     DCI_Init(const DCI_InitTypeDef *init);
void     DCI_Cmd(FunctionalState state);            /* CR.ENABLE  */
void     DCI_CaptureCmd(FunctionalState state);     /* CR.CAPTURE */
void     DCI_INTConfig(uint32_t src, FunctionalState state);   /* IER，采集中可调 */
ITStatus DCI_GetINTStatus(uint32_t src);            /* MIS */
ITStatus DCI_GetRawINTStatus(uint32_t src);         /* RIS */
void     DCI_ClearINTPendingBit(uint32_t src);      /* ICR，write-1-only */
```

### 采集层

```c
DCI_ERROR DCI_Capture_Init(const DCI_InitTypeDef *init, const DCI_DMA_config *dma);
DCI_ERROR DCI_Capture_Start(DCI_CAPTURE_MODE_T mode);   /* SNAPSHOT / CONTINUOUS */
DCI_ERROR DCI_Capture_Stop(void);       /* 优雅停：清 CAPTURE + 回读轮询 */
DCI_ERROR DCI_Capture_Abort(void);      /* 立即停；此前交付的帧立即失效，见 §11.5 契约 */
DCI_ERROR DCI_Capture_Suspend(void);    /* 手动背压，同 ST */
DCI_ERROR DCI_Capture_Resume(void);
void      DCI_Handler(void);            /* DCI 全局中断入口 */
```

### 状态查询与取帧（§11.3）

```c
DCI_CAPTURE_STATE_T DCI_Capture_GetState(void);
bool                DCI_Capture_IsFrameReady(void);
uint8_t             DCI_Capture_GetReadyCount(void);
DCI_ERROR           DCI_Capture_GetFrame(DCI_frame_t *frame);
DCI_ERROR           DCI_Capture_WaitFrame(DCI_frame_t *frame, uint32_t timeout_ms);
void                DCI_Capture_ReleaseBuffer(uint8_t index);
uint32_t            DCI_Capture_GetTransferredBytes(void);   /* JPEG / 异常诊断 */
```

### 运行时重配置（§12）

```c
DCI_ERROR DCI_Capture_Reconfig(const DCI_InitTypeDef *init, const DCI_DMA_config *dma);
DCI_ERROR DCI_Capture_SetBuffers(uint32_t *const buf[], uint8_t count);
DCI_ERROR DCI_SetCaptureRate(DCI_FCRC_T rate);
DCI_ERROR DCI_ConfigSyncCode(const DCI_SyncCode_t *code, const DCI_SyncUnmask_t *unmask);
DCI_ERROR DCI_ConfigCrop(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
DCI_ERROR DCI_CropCmd(FunctionalState state);   /* 开/关 crop 改变 frame_bytes，会失败 */
```

**`capture mode` 放 `Start` 而不是 `Init`**，`crop` 独立成 API —— 照 ST 的结构（`HAL_DCMI_Start_DMA(hdcmi, DCMI_Mode, ...)`、`HAL_DCMI_ConfigCrop()`）。理由：这两个是「每次采集会话」的选择，不是外设静态属性。

### 配置结构

```c
/* 支持的最大 framebuffer 数。定长数组而非指针，使配置结构可在 Init 时整体拷贝、
 * 不留悬空指针；将来要支持四缓冲只改这一个宏。 */
#define DCI_MAX_BUF     2
#define DCI_MAX_CHUNK   33      /* 由 spec 最大尺寸反推，BLOCK_TS 16 bit，§6.3 */
#define DCI_MAX_DESC    (DCI_MAX_BUF * DCI_MAX_CHUNK)      /* = 66，环共 1320 字节 */

typedef enum { DCI_BUF_SINGLE = 1, DCI_BUF_DOUBLE = 2 } DCI_BUF_COUNT_T;

typedef struct {
    /* --- DMA --- */
    uint8_t          dma_channel_num;   /* 建议 0/1，FIFO 深 32 */
    uint8_t          dma_priority;      /* 0 最高 */

    /* --- 缓冲 --- */
    DCI_BUF_COUNT_T  buf_count;              /* 1 .. DCI_MAX_BUF，只用前 buf_count 项 */
    uint32_t        *buf[DCI_MAX_BUF];       /* 4B 对齐即可；所在内存区驱动不关心，见 §10 */
    uint32_t         buf_bytes;              /* ≥ frame_bytes（JPEG 为容量上限）*/

    /* --- 冲突策略（§8.4）--- */
    DCI_ON_BUSY_T    on_busy;           /* GATE（默认）/ OVERWRITE */

    /* --- 分块（§6.2）--- */
    /* --- 通知（§11）--- */
    DCI_NOTIFY_MODE_T notify_mode;
    void (*frame_ready)(const DCI_frame_t *frame);      /* CALLBACK 模式必需 */
    void (*frame_error)(DCI_ERROR err, uint32_t ris);   /* 建议提供，见错误码 */
    void (*vsync_event)(void);                          /* 可选，NULL 则不使能 IT_VSYNC */
    void (*line_event)(void);                           /* 可选，NULL 则不使能 IT_LINE */
} DCI_DMA_config;
```

`buf[]` 用 `uint32_t *` 而非 `void *` 是有意的 —— 类型本身就是「必须 word 对齐」的提示。交给消费者的 `DCI_frame_t.addr` 则是 `uint8_t *`，按字节访问。

### 中断默认值

**不照抄 ST 的默认值。** 按 §11.4 的原则「没有消费者的中断不使能」：

| 中断 | 使能 |
|---|---|
| `OVR` / `ERR` | 恒开（驱动自身需要） |
| `FRAME` | 恒开（不做动态开关，§7.2） |
| `VSYNC` | 仅当 `vsync_event != NULL` |
| `LINE` | 仅当 `line_event != NULL` |

ST 的 `HAL_DCMI_Init` 无条件开 `LINE | VSYNC | ERR | OVR`，而 `LINE` 在 466 行 @60fps 下是 28k 次/秒 —— 没有回调时纯属浪费。

**块数相关的配置项已移除**：`lines_per_chunk` 和 `jpeg_chunk_bytes` 改为驱动按 §6.3 内部推导，不再暴露。

---

## 15. 参数校验清单

`DCI_Capture_Init` / `Reconfig` 应拒绝（**全部在改动硬件之前完成**，见 §12.3）：

| 条件 | 错误码 |
|---|---|
| `crop.w % 4 != 0` | 向下对齐 + 告警（spec §2.6：捕获的 pclk 数必须是 4 的倍数，行长才能被 word 整除） |
| `sensor_pclk_per_line == 0 \|\| sensor_lines == 0` | `INVALID_PARAM`（帧长无从推导，§12.6-D1） |
| 非 JPEG 且 `sensor_lines > 2048` | `SIZE` |
| 非 JPEG 且 `sensor_pclk_per_line > 4096` | `SIZE`（见下方注） |
| `edm > 1` / `fcrc > 2` / `bsm > 3` / `sync > 1` | `INVALID_PARAM` |
| `jpeg && (crop \|\| ess)` | `MODE_CONFLICT`（spec §2.7） |
| `ess && edm != 0` | `MODE_CONFLICT`（spec §2.4，内嵌码仅 8-bit） |
| `bsm != 0 && edm != 0` | `MODE_CONFLICT`（寄存器表：*valid only when EDM[1:0]=00*） |
| `line_bytes % bsm_den != 0` | `INVALID_PARAM`（抽取组跨行会让后续每一行整体偏移） |
| `crop && (crop.w == 0 \|\| crop.h == 0)` | `INVALID_PARAM` |
| `crop.w > 0x4000 \|\| crop.h > 0x4000` | `SIZE`（`CAPCNT`/`VLINE` 各 14 bit） |
| `crop.x0 + crop.w > sensor_pclk_per_line`（或纵向同理） | `SIZE`（§12.6-D2：硬件静默截断 → 失步） |
| `buf_bytes < frame_bytes`（定长） | `BUF_TOO_SMALL` |
| `buf[i] == NULL` | `INVALID_PARAM` |
| `buf[i]` 或 `buf_bytes` 非 4 字节对齐 | `ALIGN` |
| `chunks_per_frame > DCI_MAX_CHUNK`（3）或 `chunk_words > DCI_MAX_BLOCK_TS` | `SIZE` |
| `desc_count > DCI_MAX_DESC`（6） | `SIZE` |
| `buf_count` 不在 1..2 / `on_busy` / `notify_mode` 越界 | `INVALID_PARAM` |
| `dma_channel_num > 15` 或通道深度查不到 | `DMA_CHANNEL` |
| `dma_priority > 15` | `INVALID_PARAM` |
| `notify_mode == CALLBACK && frame_ready == NULL` | `NO_CALLBACK` |
| 有 buffer 被消费者持有（`Reconfig` / `SetBuffers`） | `BUSY` |

三处措辞按实现校准（原表一律写 `INVALID_PARAM`，粒度太粗）：尺寸/块数类用 `SIZE`、对齐类用 `ALIGN`、互斥组合用 `MODE_CONFLICT`、缓冲不足用 `BUF_TOO_SMALL` —— 与 §14 错误码的注释一致，调用方能直接看出该改哪一项。

> **为什么宽度校验是 pclk 而不是像素。** spec §2.9 的 2048 上限是像素数，但 pclk↔pixel 的比例取决于像素格式（RGB565/YCbCr422 在 8-bit 总线上是 2 pclk/pixel，单色 8-bit 是 1），而**格式对 DCI 不可见** —— 它只数 word。所以只能按最宽松的合法情形（2 pclk/pixel）取 `2048 × 2 = 4096` 作为 pclk 上限。这个校验是防呆，不是精确的 spec 复述。

> **crop 窗口在「设了就查」而不是「开了才查」**（实现时调整）：否则 `DCI_ConfigCrop()` 会接受一个越界窗口，等到之后 `DCI_CropCmd(ENABLE)` 才失败，调用方得自己猜是哪一步的问题。窗口非空即校验，报错落在真正出错的那次调用上。

---

## 16. 与 ST HAL 的对比

| 问题 | ST（`stm32f4xx_hal_dcmi.c`） | 本设计 |
|---|---|---|
| 一帧超单块上限 | `NDTR` 只 16 bit（65535 word）。对折成 2 的幂 + DMA 硬件双缓冲 + **中断里反复改写 `M0AR`/`M1AR`**（`:398-412`, `:1043-1072`） | `BLOCK_TS` **也是 16 bit**（同样 65535 word），但用 LLI 环按「装不下才加」拆到最多 33 块（§6.3），**跑起来不改描述符** —— 这才是本设计相对 ST 的实质优势：同样的硬件限制，ST 靠 ISR 改地址，我们靠描述符链一次配好 |
| 帧内流水 | 无。只有整帧完成通知 | 同样不做（已评估后放弃，§6.1） |
| `LINE` 中断默认值 | **无条件使能**，无回调时每秒白进 28k 次 | 仅当注册 `line_event` 才使能（§11.4） |
| JPEG 越界残留数据 | 无检测，也无交付 | 检测 + **按 `truncated` 帧交付**（§9.2） |
| 双 framebuffer | **完全不支持**。`HAL_DCMI_Start_DMA` 只收一个 `pData`。注释里的 "DOUBLE_BUFFER Mode" 指 DMA 分块，两个地址都在同一 framebuffer 内 | 支持 `buf_count = 1/2`，SNAPSHOT 与 CONTINUOUS 皆可 |
| 连续 JPEG | 无专门处理。帧长靠应用读 `NDTR`，跨帧重装也由应用做 | 归约成「驱动内循环的快照」+ 防御性关 `CAPTURE`，跳帧代替 OVR（§9.4） |
| 背压 | `HAL_DCMI_Suspend()` / `Resume()`，**手动调**（`:484-548`） | 同机制，ISR 里**自动化** |
| 帧边界确认 | 清 `CAPTURE` 后**回读轮询到 0**（`:443-457`, `:498-515`） | **照抄** |
| 块/帧交叉校验 | **动态开关 `FRAME` 中断**（`:1078` 开、`:635` 关）—— 因为 DBM 里没有"传输完成"事件，计数器绕回是唯一信号 | **不照抄**。SNAPSHOT 用 `LLP=0` 的 `GDMA_INT_Transfer` 做配对（更直接）；CONTINUOUS 让 `FRAME` 恒开做计数。ST 的做法在 `chunks_per_frame == 1` 时有静默挂死盲区（§7.2） |
| 帧偏短 / 偏长诊断 | 无。短帧静默损坏 | `FRAME_SHORT` / `FRAME_LONG`，且残留数据按 `truncated` 交付（§7.2.1） |
| OVR / ERR | `HAL_DMA_Abort_IT()` + ERROR 态 + `ErrorCallback`，**不自动恢复**（`:561-595`） | abort + 从帧边界重启，做进驱动 |
| Cache | **完全不做**，推给 MPU / 应用（AN4839） | 同思路：用户 framebuffer 的内存区驱动**不管不查**；只保证自己的描述符环对 DMA 可见（§10） |
| 通知 | weak ISR 回调，**无参数**，**无轮询接口** | 回调（带 `DCI_frame_t`）+ 轮询双通道 + 所有权协议 |
| JPEG 长度 | 不提供，应用读 `NDTR` 剩余 | 驱动内做，`GetTransferredBytes()` |
| JPEG 越界 | 无检测 | 组内块计数检测 `BUF_OVERFLOW` |
| 运行时重配置 | 无。应用自己 `Stop` + `Init` | `Reconfig`（**先校验后破坏**）+ `SetBuffers` 快路径 |
| Reg dump | 无 | §13 |

结论：**结构和错误处理照 ST**（它在真机上跑了十几年），**缓冲管理、帧内流水、诊断能力做 ST 没做的**。

---

## 17. 待验证 / 待确认

代码里统一标 `TODO(verify)` / `TODO(HAL)`。

| # | 项 | 影响 | 状态 |
|---|---|---|---|
| 1 | **DMA 握手索引** | ~~阻塞~~ **已解决** | **= 48**（2026-09-24）。依据 `BBPRO4_DmaPorts_20260915_v0.xlsx`（history `20260914_v0` 加入 AP/SCP/XP DCI）＋ SoC 侧「**AP DMA0 被拿掉，请用 AP DMA1 HS[48]**」。**表里写 49 但不能用**：那一栏按 AP DMA0/1 合并编号，而 `SRC_PER` 是每控制器本地端口号 → DMA1 上是 48。SDK 侧独立印证 DMA0 已裁：AP 核 `RCCClock_TypeDef` 只有 `DMAC1_CLOCK`、`DMA_CLOCK` 被定义成 `NULL`（SCP 核才是 `DMAC0_CLOCK`），且 `DMA_GetDMAChannelx()` 把 AP ch16~23 映射到 `DMA1_Channel0..7`（`DMA1_CH_NUM0 = 16`）。**连带**：`rtl_dci_dma_channel_alloc()` 现在强制只取 DMA1 通道（16~23）—— `hal_dma_channel_alloc()` 会忽略传入的通道号、从 0 往上扫，放任它就会分到不存在的 DMA0 |
| 2 | **驱动自己的描述符环如何保证 DMA 可见**（非缓存段放置 or 写完 clean） | `DCI_NONCACHE_SECTION` 的真实实现，见 §10.3 | `TODO(HAL)`。**2026-09-17 复查仍无**：`kernel/linker/inc/section.h` 只有放置段（`RAM_DATA_SECTION` / `BUFFER_RAM_DATA_SECTION` / `SHM_DATA_SECTION` / `RAM_NON_RETENTION_SECTION` / overlay 组），**无 cacheability 属性段、也无 DTCM/ITCM 段宏**；`fmc_platform.h` 的 `FMC_MAINx_NON_CACHE_ADDR()` 只覆盖 flash（SPIC）。**可绕过**：描述符环只在 `Init`/`Reconfig`/`SetBuffers` 时写、不是每帧写，所以「写完 clean 一次」即可，或干脆放 DTCM（TCM 不经 cache）。不阻塞 |
| 3 | ~~`rtl_8` 目录名~~ | — | **已解决**：改名为 `rtl87x3k`。DCI 只存在于 BBPro4（RTL87X3K），SDK 已确认（§2） |
| 4 | 清 `CAPTURE` 时帧尾 word 是否全部吐净 | 背压方案正确性 | ST 的回读轮询间接支持，仍需实测 |
| 5 | ~~DW DMAC「从指定 LLI 重启通道」的寄存器序列~~ | — | **已解决，不需要该原语**。连续 JPEG 的 re-arm 是 `LLP = 0` 下重调 `GDMA_Init()`（无链可跳）；定长 resync 是 `GDMA_Init()` + `Multi_Block_Struct = &ring[0]`，与初始化同一条路径，IDU 已验证 |
| 6 | ~~`CURR_DAR` vs `TRANS_DATA_CNT`~~ | JPEG 长度 | **已定**：用 `CURR_DAR` 减基址，前置条件是先等 `CFG_LOWx.FIFO_EMPTY`（bit 9）。见 §9.3 |
| 7 | **DCI 自身的** FIFO 实际深度 | 溢出余量估算 | spec 标「8-word？」，实测。（DMA 通道侧的深度已确认：`hal_dma.h` 有 `HAL_DMA_FIFO_DEPTH_16/32/64` 分档，见项 10） |
| 8 | AHB 实际可承受速率 | 分辨率/帧率上限 | spec 标「54MB/s?」，实测 |
| 9 | LLI 预取行为 | 已不阻塞 | 改用 `CAPTURE` 门控后不再改写描述符，降级为好奇项 |
| 10 | **`DEST_MSIZE` 与通道 FIFO 深度的实际配合**，以及块尾不足一个 `DEST_MSIZE` 时的补齐行为 | 写侧 AHB 突发效率（§5）；确认不破坏「块完成 = 数据已落内存」 | `TODO(verify)`。**已实现**：`rtl_dci_dma_channel_alloc()` 按 `HAL_DMA_FIFO_DEPTH_64 -> 32 -> 16 -> DEFAULT` 依次申请，把拿到的档位记进 `dci_dma_depth[ch]`，`rtl_dci_get_dma_depth()` 返回该值而非通道号硬编码表（2026-09-17 复查 `hal_dma.h:63-68` 四档枚举与 `hal_dma_channel_alloc(uint8_t*, void*, T_HAL_DMA_CHANNEL_TYPE)` 签名均未变）。注意 AP 核 `DMA_SUPPORT_MSIZE_BIGGER_THAN_FIFO_DEPTH = 0`，`DEST_MSIZE` 不得超过实际深度。**剩下要实测的**只有块尾不足一个 `DEST_MSIZE` 时的补齐行为 |
| 11 | **`CAPCNT` / `VLINE` 的编码到底是 N-1 还是「N 且硬件强制清低 2 bit」** | crop 后的实际行长，错了直接失步 | `TODO(verify)`。§2 硬约束表写「低 2 bit 被硬件强制清 0」，而 §13.4 的 dump 样例是 `CAPCNT=0xEF(240 pclk)`（N-1，未清位）。两者不能同时成立。实现按 **N-1 + 要求 pclk 数为 4 的倍数** 处理（`capcnt = w - 1`），这与 §2「4 pclk = 整数个 word」的动机一致。**`DCI_RegTest()` 顺带就能定论**（`capcnt_low_forced` 字段，§13.6），不必等 TESTPLAN 7.5 |
| 12 | **DCI 自身的时钟使能路径** | ~~阻塞~~ **不需要** | **DCI 时钟默认开启**（SoC 侧确认，2026-09-24），`rtl_dci_clock_enable()` 保持空实现、真实使能语句以注释保留。寄存器块当下即可访问，所以 `reg-test` 与 `snapshot + NO_CAPTURE=1` 今天就能实测。**若将来某个 part 真的要门控**：AP 核 `RCCClock_TypeDef` 里 DCI/IDU/PPE/LCDC/MIPI/JPEG/PNG/GPU 全无，所以不会是往那个枚举里加一项；display island 有自己的 `DISPLAY_AON(0x40645000)`/`DISPLAY_PLATFORM(0x40646000)`/`DISPLAY_PCC(0x40647000)`/`DISPLAY_RAP(0x4064a000)`，而 `DISPLAY_PCC_BASE` 全树只出现在 `address_map.h` 自己（无寄存器表、无使用者）→ 届时大概率是**直写 display PCC**，不是 `RCC_ClockCmd()`。另注：本 SoC 上 `RCC_PeriphClockCmd(p,c,s)` 被定义为 `RCC_ClockCmd(p,s)`，单参数形式才是原生的 |
| 12b | **GDMA 控制器时钟用错** | 曾是**静默失效** | **已修**：原来调 `RCC_ClockCmd(DMA_CLOCK, ENABLE)`，而 AP 核上 `DMA_CLOCK` 被定义成 `NULL`（`soc/driver/rcc/.../rtl_rcc_def.h:178`），整句编译通过、运行无效、什么都没开。现改为 `DMA1_CLOCK` —— DCI 走 DMA1，这也是 AP 核上唯一存在的 GDMA 时钟 |
| 13 | **DCI 的引脚复用**（D[9:0] / PCLK / HSYNC / VSYNC） | **FPGA 阶段不需要配置**；ASIC 待确认 | **已查明（2026-09-24）**：依据 `BBpro4_VU13P_FPGA_Display_FMC1DQ16_Pin_Assignment_20260915_v0.xlsx`（sheet「新版J4」，history `20260911_v1` "add DCMI signals"）—— FPGA 上 DCI 是**顶层专用 RTL 端口**，由 `.xdc` 钉到固定物理脚，**不经过 SoC pinmux**，所以 `rtl_dci_pinmux_init()` 空实现是**正确实现**而非占位。证据：①「NOTE」第 20 行说明 `Pin Name` 就是 RTL 端口名，而 DCI 信号以 `dcmi_d[13:0]`/`dcmi_pixclk`/`dcmi_hsync`/`dcmi_vsync` 出现在该列；②这些名字自成一类 —— 同表其余脚位是 `P_GPIOx_y`（39 个，SoC GPIO，才需要 mux）或 `FPGA_IO_PIN_AP/BT/WIFI[n]`（42 个，可复用 IO），无一个 `dcmi_*` 是 GPIO 脚；③`dcmi_pixclk`（FPGA `BB9`，J4 `G5`）落在 **clock-capable 脚**（标 `*P_*_GC_*`），正是外部像素时钟需要的。与 SDK 自洽：`pin_def.h` 根本没有 DCI/DCMI/DVP 功能号（止于 `SPIC0_SIO3(72)`），若要过 mux 则必须有。**ASIC 侧仍未确认** —— FPGA 板证明不了 ASIC 的 pad ring，17 根信号走通用 mux 代价高，专用 pad 仍是大概率，但上 ASIC 前要对 BBPro4 pin list 核一次 |
| 13b | **引脚表有 14 根数据线，spec 只有 10 根** | 不影响代码 | 引脚表布了 `dcmi_d[13:0]`，而 spec 明确「DCI 拥有 **10 条数据线 D[9:0]**」、`EDM` 只用 `EDM[0]` 一位、且「使用位数少于 10 则**必须将未使用的输入引脚接地**」。两者都不是笔误：**板上多布了 4 根余量，DCI 用不到**。所以 §2 里「只有 D[9:0]、12/14-bit 是 STM32 残留」的判断**成立**，枚举只暴露 8/10-bit 是对的。`d[13:10]` 视为板级预留 |
| 14 | **`DMA_Init()` 是否会把 `LLPx` 指向首描述符本身还是它的 `LLP`** | 首块是否被执行两次 | 已规避，不阻塞。`dci_arm()` 让通道寄存器与组首描述符**内容一致**，两种约定得到的块序列完全相同（IDU 也是这个用法）。真机上顺带确认即可 |
| ~~15~~ | ~~API 前缀从 `GDMA_*` 迁到 `DMA_*`~~ | — | **已完成**。新增 `rtl87x3k/rtl_dci_dma.{h,c}` 作为 DMA 抽象层，`rtl_common` 不再出现任何 SoC 名字。通道改用 `hal_dma_channel_alloc()` 按 FIFO 深度申请（取代按通道号硬编码深度表），向量注册用 `ram_vector_table_update(DCI_IRQn, ...)` |
| 16 | ~~**`BLOCK_TS` 字段宽度**~~ | 块数推导 | **已定，且与原假设不同**：**16 bit**（65,535 word = 256 KB），不是 rtl87x3g 的 20 bit。`DCI_MAX_CHUNK` 由 3 改为 33，见 §6.1 的警示框与 §6.3。SoC 层有 `_Static_assert` 钉住这个值 |

---

## 18. 实现顺序

**四种模式全部纳入首版**：定长 SNAPSHOT / 定长 CONTINUOUS / JPEG SNAPSHOT / JPEG CONTINUOUS。

1. ✅ `rtl_dci_def.h` —— 寄存器映射 + 位域 union（gcc 校验过位域布局）
2. ✅ `rtl_dci.h` —— 错误码、枚举、`InitTypeDef`、`DMA_config`、`frame_t`、`stats_t`、内联 cmd
3. ✅ `rtl_dci_int.{h,c}` —— SoC 钩子 + HAL 占位 + `DCI_NONCACHE_SECTION` + `rtl_dci_lock/unlock`
4. ✅ `rtl_common/rtl_dci.c` —— `DCI_Init` / crop / 中断使能策略（§11.4）
5. ✅ `rtl_common/rtl_dci.c` —— 描述符构建：CONTINUOUS 闭环 + SNAPSHOT 的 `LLP=0` 链（§6.4）
6. ✅ `rtl_common/rtl_dci.c` —— **SNAPSHOT 通路**：`Start/Stop` + `IT_FRAME` 收尾（等 `FIFO_EMPTY` → `CURR_DAR`）+ `Transfer` 超量告警 + `truncated` 交付（§7.2 / §7.2.1）
7. ✅ `rtl_common/rtl_dci.c` —— **CONTINUOUS 定长通路**：`evt` 会合 + 偏短偏长检测 + resync（§7.2）
8. ✅ `rtl_common/rtl_dci.c` —— ready 队列 + 轮询接口（§11.3）+ 背压门控（§8.2）
9. ✅ `rtl_common/rtl_dci.c` —— **JPEG SNAPSHOT**：复用第 6 步，只差"无期望长度"（§9.2 / §9.3）
10. ✅ `rtl_common/rtl_dci.c` —— **JPEG CONTINUOUS**：第 9 步 + ISR 内自动重装 + 防御性关 `CAPTURE`（§9.4）
11. ✅ `rtl_common/rtl_dci.c` —— `Reconfig` / `SetBuffers`（§12）
12. ✅ reg dump（§13）
13. 各级 `CMakeLists.txt` + `Kconfig` 加 `CONFIG_REALTEK_DISPLAY_ENABLE_DCI`

第 9、10 步的成本很低 —— JPEG 快照与定长快照共用第 6 步的代码，连续 JPEG 又只是快照加一句自动重启。第 7 步（会合）反而是四种模式里唯一需要独立机制的。

### 实现过程中对设计的修正

代码写出来后暴露了 5 处设计层的问题，均已回填到对应章节：

| # | 问题 | 修正 | 章节 |
|---|---|---|---|
| 1 | 门控的「回读轮询到 0」在 ISR 里最坏要自旋一整帧 | 轮询上限分线程/ISR 两档，ISR 档是尽力确认、超时不影响状态机 | §8.2 |
| 2 | resync 无条件重启到 group 0，可能覆写消费者正持有的 buf0 | resync 先过门控检查，被持有则 SUSPENDED 等 `ReleaseBuffer()` | §7.2.1 |
| 3 | 「resync 时 `ready_head` 归 0」会打乱已入队帧的取出顺序 | `ready_head` 由「队列空则定到本次交付的 buffer」自我对齐，不需复位 | §11.3 |
| 4 | SNAPSHOT 出错时不轮转 `cur_group`，重拍会撞在消费者手上那块 | 成功与失败统一轮转 | §7.2.1 |
| 5 | crop 窗口只在 `CROP=1` 时校验，`ConfigCrop()` 会接受越界窗口、错误延迟到 `CropCmd()` | 窗口非空即校验 | §15 |

另有两处是漏写而非错写：`buf_bytes >= frame_bytes`（§6.7 的核心不变式）在 §15 有、代码初版漏了，已补；§15 的错误码粒度按 §14 的分类校准。

### 主机侧验证

`rtl_common/rtl_dci.c` 与 SoC 无关，可以在 PC 上跑。做法是把 `DCI_REG_BASE` / `GDMA_REG_BASE` 重定向到 RAM 数组、桩掉 GDMA HAL 与 `rtl_dci_int.c` 的钩子，然后直接 `#include` 驱动源码。覆盖到：

- **几何推导**：466×466 RGB565（2 块 × 233 行）、512×512 RGB565（3 块，171+171+170 行，末块偏短）、2048×2048 单色（17 块）、2048×2048 RGB565（33 块 × 63 行 = spec 最坏情形）、466 宽单色（行长非 4 倍数 → 退化为 word 对齐）、`LSM`/`OELS` 奇偶行、四档 `BSM`、`EDM=1` 双字节
- **`EDM=1`（10-bit）专项**：应用层 demo 已补两行 10-bit signal（`QVGA_RAW10` 单块、`VGA_RAW10` 3 块）与一个专用 case（`DCI_DEMO_CASE_RAW10`，逐行用意见 `test/TESTMATRIX.md` §7）。为何值得单独一个 case：**10-bit 是唯一具备「与画面内容无关的逐样本不变式」的格式** —— 每个样本占一个 16-bit 半字的低 10 位、bit15:10 必为 0（§2 数据摆放），可逐样本穷举，且**坏样本比例直接指出病因**：100% = `EDM` 未生效（实际在跑 8-bit），50% = 半字相位错一字节（分块边界落在样本中间）。另外 `bsm != 0 && edm != 0` 与 `sync == EMBEDDED && edm != 0` 两条互斥拒绝由该 case 直接断言（不需摄像头），因为它们是最容易默默退化的校验 —— 一旦放宽，症状是图像错位而人会去查接线
- **校验**：`BUF_TOO_SMALL` / `ALIGN` / `MODE_CONFLICT` / `NO_CALLBACK` / `DMA_CHANNEL` / `SIZE`（含 crop 越界横纵两向）/ `BSM` 不整除
- **拓扑**：CONTINUOUS 闭环的 `LLP` 首尾相接、SNAPSHOT 每组末 `LLP=0` 且清掉 LLI 位、单 buffer 单块自环、末块偏短、`Σ CTL_HI == frame_words × buf_count`、通道寄存器与组首描述符一致
- **交付与背压**：入队即上锁、门控在交付返回之后、`ReleaseBuffer` 唤醒、`OVERWRITE` 下 `frames_dropped` 与队列顶掉最旧帧、POLL 队列的 FIFO 次序
- **四种模式的收尾**：定长快照正常/偏短/偏长、JPEG 快照正常/越界、连续 JPEG 自动重装与门控、会合两种到达顺序 + 同 bit 两次触发 resync
- **重配置**：持锁时返回 `BUSY` 且不动硬件、参数非法时不影响正在跑的采集、纯软件字段不重装 DMA、`Abort` 强行回收、mode 首次 `Start` 后锁定
- **reg dump**：RESET 与运行两种状态下的 `RegSnapshot`/`DumpSnapshot`/`RegMonitor`
