# DCI 裸机测试工程：架构设计

## 1. 目标与约束

在**裸机环境**（无 OS、无线程、无信号量）下打通并验证 DCI 全链路：

```
camera IO 初始化 → camera 寄存器配置 → framebuffer 准备
   → DCI 初始化配置 → 启动 capture → 完成 capture → dump 数据
```

四个待测项，编译期宏选择，**互不干扰**：

| 待测项 | 格式 | 采集模式 | DCI 拓扑 |
|---|---|---|---|
| `TC_RGB_SNAPSHOT` | RGB565 定长 | SNAPSHOT | `LLP=0` 独立链 |
| `TC_RGB_CONTINUOUS` | RGB565 定长 | CONTINUOUS | **闭环**（唯一需要 `evt` 会合的） |
| `TC_JPEG_SNAPSHOT` | JPEG 变长 | SNAPSHOT | `LLP=0` 单块 |
| `TC_JPEG_CONTINUOUS` | JPEG 变长 | CONTINUOUS | `LLP=0` + ISR 内自动重装 |

约束：

| 约束 | 影响 |
|---|---|
| 无 OS | 通知走 `DCI_NOTIFY_POLL`（`WaitFrame()` 是 busy-wait，本就为无 OS 设计）；主循环轮询 |
| framebuffer 数量可配 | `buf_count ∈ {1, 2}`（`DCI_MAX_BUF = 2`），与 `on_busy` 策略联动 |
| 输出尺寸由宏控制 | **必须同时生效于 camera 寄存器配置和 DCI 声明几何** —— 见 §4，这是整个工程最容易出错的地方 |
| dump 走 UART | 466×466 RGB565 = 434 KB，全量 hex 约 7 s @1 Mbps → dump 必须分级（§8） |
| GDMA 握手索引仍是 `0xFF` 占位 | **capture 目前跑不通**，但 §9 的前两级 bring-up 不需要 DMA，今天就能跑 |

相关文档：设计与实现见 [DESIGN.md](DESIGN.md)，官方 73 条用例整理见 [TESTPLAN.md](TESTPLAN.md)。本工程覆盖 TESTPLAN 的 B 层（常规激励）与 §16 的驱动层用例。

### 1.1 本文档 vs 已落地的代码

本文档是**计划**，`test/` 下已落地的是**其中 DCI 一侧的部分**，两者命名不同，别混：

| | 本文档规划 | 已实现 |
|---|---|---|
| 文件前缀 | `dci_test_*` | `dci_demo_*` |
| 入口 | `dci_test_main()` | `dci_demo_run(void)` |
| 用例数 | 4 项（RGB/JPEG × 快照/连续） | 13 个 case，上述 4 项由 case × signal × overlay 组合覆盖 |
| 配置真源 | `dci_test_config.h` | `dci_demo_config.h` |
| camera 驱动 | `device/general/camera/` | **未实现** —— 仍按 §3 的规划做 |
| 尺寸真源 | `CAM_OUT_WIDTH/HEIGHT` 双向生效（§4） | signal 表只驱动 DCI 一侧 |

**差异的原因**：已实现的部分把「测什么」（case）、「引脚上是什么」（signal）与
「收下来之后再做什么」（overlay）拆成三个维度，
4 项待测项落在这些格子里，加一种配置不用加一个用例文件。

**仍然成立的部分**：§4 的「输出尺寸必须同时生效于 camera 寄存器配置和 DCI 声明几何」是
本文档最关键的一条，而这一半（camera 侧）还没有代码 —— 现在 signal 表里的尺寸只告诉 DCI
「帧应该多大」，没有任何东西保证 sensor 真的按这个尺寸输出。所以 §9 的 bring-up 阶梯里
`probe-signal` 那一步不能跳（见 [test/README.md](test/README.md) 的上板顺序表）。

实际操作说明看 [test/README.md](test/README.md)，本文档看架构依据。

---

## 2. 四个测试项的差异矩阵

**分隔设计的依据**：把差异先列清楚，才知道哪些该共用、哪些该分开。四项只在 8 处不同，其余全部相同：

| 差异点 | RGB 快照 | RGB 连续 | JPEG 快照 | JPEG 连续 |
|---|---|---|---|---|
| camera 模式表 | RGB565 | RGB565 | JPEG | JPEG |
| `init.jpeg` | `false` | `false` | `true` | `true` |
| `Start(mode)` | SNAPSHOT | CONTINUOUS | SNAPSHOT | CONTINUOUS |
| `buf_bytes` | `= frame_bytes` | `= frame_bytes` | 容量上限 | 容量上限 |
| 期望帧长 | 固定，可断言 | 固定，可断言 | **未知**，只能查合理性 | **未知** |
| 采集帧数 | 1 | `DCI_TC_FRAMES` | 1 | `DCI_TC_FRAMES` |
| 结束方式 | `CAPTURE` 硬件自清 | 显式 `Stop()` | 硬件自清 | 显式 `Stop()` |
| 特有校验 | `bytes == frame_bytes`、`!truncated` | `seq` 连号、`index` 轮转 | SOI/EOI、补零字节数 | 每帧 SOI/EOI、帧长逐帧变化 |

**相同的部分**（应当共用，绝不复制粘贴）：**DCI 寄存器自检**、IO 初始化、I2C 初始化、sensor chip ID 校验、几何交叉校验、framebuffer 分配与哨兵、`DCI_Capture_Init` 调用、中断挂接、取帧与释放、CRC 与场景无关校验、dump、统计断言、失败时 reg dump。

结论：**用「共用步骤函数 + 每项一个线性脚本」的结构**，而不是一个塞满 `#ifdef` 的大函数。全工程只在用例注册表处有一个 `#if`（§5）。

---

## 3. 工程结构

```
device/general/camera/                        ← 新增，照 device/general/touch/ 的范式
├── CMakeLists.txt
├── camera_if.h                    ★ 与 sensor 无关的接口：init / set_mode / read_id / test_pattern
└── 8773G/
    ├── CMakeLists.txt
    ├── camera_<part>.h              引脚定义、I2C 地址、chip ID、支持的模式枚举
    ├── camera_<part>.c              IO/I2C 初始化 + 寄存器表 + 模式切换 + ID 读回
    └── camera_<part>_regs.h         寄存器表（RGB565 各分辨率 / JPEG / test pattern）

hardware/dci/test/                            ← 测试工程
├── CMakeLists.txt
├── README.md                        用例编号 ↔ 函数名映射、执行与判读方式
├── dci_test.h                       对外唯一入口：void dci_test_main(void);
├── dci_test_config.h              ★ 全部配置宏 + 派生量 + 编译期一致性检查（单一真源）
├── dci_test_main.c                 用例选择与调度，全工程唯一的用例 #if
├── common/
│   ├── tc_step.h / tc_step.c        7 个基本步骤，四个用例共用
│   ├── tc_fb.h   / tc_fb.c          framebuffer 分配、哨兵、cache 维护
│   ├── tc_dump.h / tc_dump.c        分级 dump + CRC32 + JPEG 结构检查
│   └── tc_check.h/ tc_check.c       场景无关的自动校验 + 统计断言
└── cases/
    ├── tc_rgb_snapshot.c            四个文件互不 include，各自线性可读
    ├── tc_rgb_continuous.c
    ├── tc_jpeg_snapshot.c
    └── tc_jpeg_continuous.c
```

**为什么 camera 驱动不放 `hardware/dci/test/` 里**：它是设备驱动，可复用（将来实际产品也要用），且仓库已有 `device/general/touch/<SoC>/` 的先例。测试工程只依赖 `camera_if.h`，换 sensor 不动测试代码。

**入口约定**：`dci_test_main()` 由 SDK app 的 `main()` 在时钟/UART 就绪后调用，本工程不接管系统初始化。

---

## 4. 配置宏体系：单一真源 ★

这是整个工程最关键的设计点。**输出尺寸必须同时生效于两处**：

```
                    CAM_OUT_WIDTH / CAM_OUT_HEIGHT
                              │
              ┌───────────────┴───────────────┐
              ▼                               ▼
   camera 寄存器配置                  DCI_InitTypeDef
   （sensor 实际输出多大）      sensor_pclk_per_line / sensor_lines
                                    （驱动认为帧多大）
              └───────────────┬───────────────┘
                              ▼
                    两者不一致 = 必然失步
              （DESIGN.md §7.1：帧偏短/偏长 → FRAME_SHORT / FRAME_LONG / DESYNC）
```

DCI 无法从自身寄存器得知帧多大（`CROP=0` 时捕获的是 `HSYNC`/`VSYNC` 界定的任意大小），所以几何只能靠应用声明 —— **一旦声明与 sensor 实际输出不符，驱动一定报错，但报的是症状不是病因**。因此配置宏必须是单一真源，且要有三道交叉校验（§4.2）。

### 4.1 `dci_test_config.h` 骨架

```c
/*==== 1. 选测试项：四选一，必须且只能选一个 ====*/
#define DCI_TC_RGB_SNAPSHOT       1
#define DCI_TC_RGB_CONTINUOUS     0
#define DCI_TC_JPEG_SNAPSHOT      0
#define DCI_TC_JPEG_CONTINUOUS    0

/*==== 2. sensor 输出尺寸：唯一真源，改这里 ====*/
#define CAM_OUT_WIDTH             320     /* 像素 */
#define CAM_OUT_HEIGHT            240     /* 行   */

/*==== 3. framebuffer ====*/
#define DCI_TC_BUF_COUNT          2       /* 1 或 2 */
#define DCI_TC_JPEG_BUF_KB        64      /* JPEG 容量上限，仅 JPEG 用例生效 */
#define DCI_TC_FB_BUDGET_KB       512     /* 总预算，超了编译期就报错 */

/*==== 4. 策略与运行参数 ====*/
#define DCI_TC_ON_BUSY            DCI_ON_BUSY_GATE
#define DCI_TC_NOTIFY             DCI_NOTIFY_POLL
#define DCI_TC_DMA_CHANNEL        0       /* 建议 0/1，FIFO 深 32 */
#define DCI_TC_DMA_PRIORITY       0
#define DCI_TC_FRAMES             8       /* 连续模式采多少帧后 Stop */
#define DCI_TC_TIMEOUT_MS         3000
#define DCI_TC_DUMP_LEVEL         1       /* 见 §8 */
#define DCI_TC_SENSOR_TESTPAT     0       /* 1 = 开 sensor 内建测试图案 */

/*==== 5. 极性与格式（按实际接线/sensor 手册填）====*/
#define DCI_TC_PCKPOL             false   /* false = 下降沿 */
#define DCI_TC_HSPOL              false   /* false = HSYNC 低有效 */
#define DCI_TC_VSPOL              false
#define DCI_TC_EDM                DCI_EDM_8BIT

/*==== 6. 派生量：不要手改 ====*/
#define DCI_TC_IS_JPEG            (DCI_TC_JPEG_SNAPSHOT || DCI_TC_JPEG_CONTINUOUS)
#define DCI_TC_IS_CONT            (DCI_TC_RGB_CONTINUOUS || DCI_TC_JPEG_CONTINUOUS)

#if DCI_TC_IS_JPEG
/* JPEG 是字节流，帧长事先未知；几何字段仅用于日志与校验（驱动只查非 0） */
#define DCI_TC_PCLK_PER_PIXEL     1
#define DCI_TC_FRAME_BYTES        0                              /* 无期望长度 */
#define DCI_TC_BUF_BYTES          (DCI_TC_JPEG_BUF_KB * 1024)
#define DCI_TC_CAM_FORMAT         CAMERA_FMT_JPEG
#else
/* RGB565 走 8-bit 总线：一个像素两个 pclk */
#define DCI_TC_PCLK_PER_PIXEL     2
#define DCI_TC_FRAME_BYTES        ((uint32_t)CAM_OUT_WIDTH * CAM_OUT_HEIGHT * 2)
#define DCI_TC_BUF_BYTES          DCI_TC_FRAME_BYTES
#define DCI_TC_CAM_FORMAT         CAMERA_FMT_RGB565
#endif

#define DCI_TC_PCLK_PER_LINE      (CAM_OUT_WIDTH * DCI_TC_PCLK_PER_PIXEL)
#define DCI_TC_FRAMES_WANTED      (DCI_TC_IS_CONT ? DCI_TC_FRAMES : 1)
#define DCI_TC_FB_TOTAL_KB        ((DCI_TC_BUF_BYTES * DCI_TC_BUF_COUNT) / 1024)

/*==== 7. 编译期一致性检查 ====*/
#define DCI_TC_SEL_SUM  (DCI_TC_RGB_SNAPSHOT + DCI_TC_RGB_CONTINUOUS + \
                         DCI_TC_JPEG_SNAPSHOT + DCI_TC_JPEG_CONTINUOUS)
_Static_assert(DCI_TC_SEL_SUM == 1,           "exactly one test case must be selected");
_Static_assert(DCI_TC_BUF_COUNT >= 1 && DCI_TC_BUF_COUNT <= DCI_MAX_BUF, "buf_count 1..2");
_Static_assert((DCI_TC_BUF_BYTES & 3U) == 0,  "buf_bytes must be 4-byte aligned");
_Static_assert(DCI_TC_FB_TOTAL_KB <= DCI_TC_FB_BUDGET_KB, "framebuffers exceed budget");
#if !DCI_TC_IS_JPEG
_Static_assert(CAM_OUT_HEIGHT <= 2048,        "spec 2.9: non-JPEG max 2048 lines");
_Static_assert(DCI_TC_PCLK_PER_LINE <= 4096,  "spec 2.9: non-JPEG max 2048 px/line");
#endif
```

三点说明：

- **JPEG 下 `sensor_pclk_per_line`/`sensor_lines` 仍要填真实值。** 驱动对 JPEG 只校验它们非 0（`dci_calc_geometry()` 在 `jpeg` 分支提前返回），但填真实值能让日志有意义，且切回 RGB 用例时不用改两处。
- **`FB_BUDGET` 是必要的。** 2 × 466×466×2 = 848 KB，多数 MCU 的 SRAM 装不下。编译期拦住比运行时莫名跑飞好。bring-up 应从 320×240（150 KB）起步。
- **RGB565 的 `PCLK_PER_PIXEL = 2` 依赖「8-bit 总线」这个前提。** 若接的是 10-bit raw（`EDM=1`），换算规则不同，要一并改 §5 的 `DCI_TC_EDM` 与此处 —— 这两个宏有耦合，注释里必须写明。

### 4.2 三道几何交叉校验

单一真源只能保证「代码里写的一致」，不能保证「sensor 真的这么输出」。三道校验，成本递增、能力递增：

| 级别 | 手段 | 能查出 | 何时执行 |
|---|---|---|---|
| **C1 编译期** | camera 驱动导出支持的模式列表，配置头断言 `CAM_OUT_WIDTH×HEIGHT` 在列表内 | 尺寸写错、模式表缺失 | 编译 |
| **C2 寄存器读回** | 配完 sensor 后读回其输出尺寸寄存器，与宏比对 | **模式表内容与宏不符** | camera init 之后 |
| **C3 信号实测** ★ | 两次 `VSYNC` 之间数 `LINE` 中断次数 = **实际行数** | 上面两道都测不到的：模式表对但 sensor 未生效、寄存器读回值不代表真实输出 | capture 之前 |

**C3 是最有价值的一道**，因为它直接测量硬件的真实行为，且**完全不需要 DMA** —— 只注册 `vsync_event` 和 `line_event` 两个回调即可。它把「集成阶段最常见的错误」（声明几何 ≠ 实际输出）从「capture 后收到 `FRAME_SHORT` 再猜原因」提前到「capture 前就明确告知实际是多少行」。

```c
/* tc_step.c —— C3：不搬一个字节，只数中断 */
static volatile uint32_t s_vsync_cnt, s_line_cnt, s_lines_measured;

static void on_vsync(void)
{
    if (s_vsync_cnt != 0) { s_lines_measured = s_line_cnt; }   /* 完整一帧的行数 */
    s_line_cnt = 0;
    s_vsync_cnt++;
}
static void on_line(void) { s_line_cnt++; }

tc_result_t tc_step_probe_signal(void)
{
    /* 只开 VSYNC/LINE 回调，不启 DMA、不置 CAPTURE */
    ... DCI_Capture_Init(&init, &dma_probe_only) ...
    ... 等若干帧 ...

    TC_LOG("[probe] vsync=%u  lines/frame measured=%u  declared=%u",
           s_vsync_cnt, s_lines_measured, CAM_OUT_HEIGHT);

    if (s_vsync_cnt == 0)  return TC_FAIL_NO_VSYNC;      /* 接线/极性/时钟问题 */
    if (s_lines_measured == 0) return TC_FAIL_NO_LINE;   /* HSYNC 极性问题 */
    if (s_lines_measured != CAM_OUT_HEIGHT)
        return TC_FAIL_GEOMETRY;                         /* ★ 病因直接给出 */
    return TC_PASS;
}
```

> `LINE` 中断在 466 行 @60fps 下是 28k 次/秒，只在这一步开、探测完立刻关（把 `line_event` 置 NULL 后 `Reconfig`），正式采集不开 —— 这也顺带验证了驱动「没有回调就不使能」的策略（DESIGN.md §11.4）。

---

## 5. 测试项的分隔机制

### 5.1 七个共用步骤

`tc_step.h` 暴露的全部接口，四个用例都从这里取积木：

```c
typedef enum {
    TC_PASS = 0,
    TC_FAIL_REG_DEAD, TC_FAIL_REG_BITS,
    TC_FAIL_I2C, TC_FAIL_CHIP_ID, TC_FAIL_SENSOR_CFG,
    TC_FAIL_NO_VSYNC, TC_FAIL_NO_LINE, TC_FAIL_GEOMETRY,
    TC_FAIL_DCI_INIT, TC_FAIL_START, TC_FAIL_TIMEOUT,
    TC_FAIL_FRAME_LEN, TC_FAIL_TRUNCATED, TC_FAIL_GUARD,
    TC_FAIL_CONTENT, TC_FAIL_STATS,
} tc_result_t;

tc_result_t tc_step_regtest(void);                       /* L0 寄存器自检，无 DMA */
tc_result_t tc_step_camera_init(void);                   /* IO + I2C + chip ID (C2) */
tc_result_t tc_step_camera_mode(camera_format_t fmt);    /* 寄存器表下发 + 读回校验 */
tc_result_t tc_step_probe_signal(void);                  /* C3 几何实测，无 DMA */
tc_result_t tc_step_fb_prepare(void);                    /* 分配 + 哨兵 + 清零 */
tc_result_t tc_step_dci_init(bool jpeg);                 /* 组 InitTypeDef/DMA_config */
tc_result_t tc_step_capture_start(DCI_CAPTURE_MODE_T m);
tc_result_t tc_step_wait_frame(DCI_frame_t *f, uint32_t timeout_ms);
```

`tc_step_regtest()` 就是一层薄封装，把驱动的两个错误码翻译成用例的返回值 —— 分成两个是因为处理动作完全不同：

```c
tc_result_t tc_step_regtest(void)
{
    switch (DCI_RegTestDump())              /* 报告已在里面打印 */
    {
    case DCI_SUCCESS:              return TC_PASS;
    case DCI_ERROR_NO_RESPONSE:    return TC_FAIL_REG_DEAD;   /* 查时钟、查基址 */
    default:                       return TC_FAIL_REG_BITS;   /* 查位域定义 */
    }
}
```

`tc_step_dci_init()` 内部把 §4 的派生宏映射到驱动结构，**这是宏与驱动的唯一接缝**：

```c
tc_result_t tc_step_dci_init(bool jpeg)
{
    DCI_InitTypeDef init;
    DCI_StructInit(&init);
    init.sensor_pclk_per_line = DCI_TC_PCLK_PER_LINE;   /* ← 单一真源在此生效 */
    init.sensor_lines         = CAM_OUT_HEIGHT;
    init.edm    = DCI_TC_EDM;
    init.jpeg   = jpeg;
    init.pckpol = DCI_TC_PCKPOL;
    init.hspol  = DCI_TC_HSPOL;
    init.vspol  = DCI_TC_VSPOL;
    /* init.crop 保持 false：Capture_Init 不接受 crop，窗口走 ConfigCrop + CropCmd */

    DCI_DMA_config dma = {
        .dma_channel_num = DCI_TC_DMA_CHANNEL,
        .dma_priority    = DCI_TC_DMA_PRIORITY,
        .buf_count       = (DCI_BUF_COUNT_T)DCI_TC_BUF_COUNT,
        .buf_bytes       = DCI_TC_BUF_BYTES,
        .on_busy         = DCI_TC_ON_BUSY,
        .notify_mode     = DCI_TC_NOTIFY,
        .frame_error     = tc_on_frame_error,           /* 恒装，用于抓错误码 */
#if (DCI_TC_NOTIFY == DCI_NOTIFY_CALLBACK)
        .frame_ready     = tc_on_frame_ready,
#endif
        /* vsync_event / line_event 留 NULL：正式采集不开这两个中断 */
    };
    for (uint8_t i = 0; i < DCI_TC_BUF_COUNT; i++) { dma.buf[i] = tc_fb_get(i); }

    return (DCI_Capture_Init(&init, &dma) == DCI_SUCCESS) ? TC_PASS : TC_FAIL_DCI_INIT;
}
```

### 5.2 用例注册：全工程唯一的 `#if`

```c
/* dci_test_main.c */
typedef struct {
    const char *name;
    tc_result_t (*run)(void);
} tc_entry_t;

/* 四个用例的 run() 各自在 cases/*.c 里定义，互不 include */
extern tc_result_t tc_rgb_snapshot_run(void);
extern tc_result_t tc_rgb_continuous_run(void);
extern tc_result_t tc_jpeg_snapshot_run(void);
extern tc_result_t tc_jpeg_continuous_run(void);

static const tc_entry_t s_case =
#if   DCI_TC_RGB_SNAPSHOT
    { "RGB565 / SNAPSHOT",   tc_rgb_snapshot_run };
#elif DCI_TC_RGB_CONTINUOUS
    { "RGB565 / CONTINUOUS", tc_rgb_continuous_run };
#elif DCI_TC_JPEG_SNAPSHOT
    { "JPEG / SNAPSHOT",     tc_jpeg_snapshot_run };
#elif DCI_TC_JPEG_CONTINUOUS
    { "JPEG / CONTINUOUS",   tc_jpeg_continuous_run };
#endif

void dci_test_main(void)
{
    tc_banner(s_case.name);                  /* 打印全部生效配置，见 §5.4 */
    tc_result_t r = s_case.run();
    tc_verdict(s_case.name, r);              /* 失败则自动 DCI_RegDump() */
}
```

**四个 `run()` 全部编译进去，只有一个被引用** —— 链接器丢掉未引用的。好处：换用例只改一个宏，但**四个用例始终都被编译**，不会出现「改了共用代码，另外三个用例编译不过而没人发现」。这是 `#ifdef` 包住整段代码所做不到的。

### 5.3 一个用例文件的样子

线性可读，从上到下就是测试步骤，可在任意一行下断点：

```c
/* cases/tc_rgb_snapshot.c —— RGB565 快照 */
tc_result_t tc_rgb_snapshot_run(void)
{
    DCI_frame_t f;
    tc_result_t r;

    TC_STEP(0, "dci reg test");       if ((r = tc_step_regtest()) != TC_PASS) return r;
    TC_STEP(1, "camera init");        if ((r = tc_step_camera_init()) != TC_PASS) return r;
    TC_STEP(2, "camera mode RGB565"); if ((r = tc_step_camera_mode(CAMERA_FMT_RGB565)) != TC_PASS) return r;
    TC_STEP(3, "probe geometry");     if ((r = tc_step_probe_signal()) != TC_PASS) return r;
    TC_STEP(4, "framebuffer");        if ((r = tc_step_fb_prepare()) != TC_PASS) return r;
    TC_STEP(5, "dci init");           if ((r = tc_step_dci_init(false)) != TC_PASS) return r;
    TC_STEP(6, "capture start");
    if ((r = tc_step_capture_start(DCI_MODE_SNAPSHOT)) != TC_PASS) return r;

    TC_STEP(7, "wait frame");
    if ((r = tc_step_wait_frame(&f, DCI_TC_TIMEOUT_MS)) != TC_PASS) return r;

    /*---- 本用例特有的校验：定长格式，帧长可精确断言 ----*/
    TC_STEP(8, "verify");
    TC_EXPECT_EQ(f.bytes, DCI_TC_FRAME_BYTES, TC_FAIL_FRAME_LEN);
    TC_EXPECT(!f.truncated,                   TC_FAIL_TRUNCATED);
    TC_EXPECT(tc_check_not_flat(&f),           TC_FAIL_CONTENT);   /* 非全 0/全同值 */
    if ((r = tc_check_guards()) != TC_PASS) return r;
    if ((r = tc_check_stats_clean(1)) != TC_PASS) return r;        /* 只应 frames_done=1 */

    TC_STEP(9, "dump");
    tc_dump_frame(&f, DCI_TC_DUMP_LEVEL);
    DCI_Capture_ReleaseBuffer(f.index);

#if (DCI_TC_BUF_COUNT == 2)
    /*---- 双 buffer 下再拍一张，验证驱动内部轮转（DESIGN.md §6.4）----*/
    TC_STEP(10, "second shot rotates buffer");
    uint8_t first_idx = f.index;
    if ((r = tc_step_capture_start(DCI_MODE_SNAPSHOT)) != TC_PASS) return r;
    if ((r = tc_step_wait_frame(&f, DCI_TC_TIMEOUT_MS)) != TC_PASS) return r;
    TC_EXPECT(f.index != first_idx, TC_FAIL_CONTENT);
    tc_dump_frame(&f, DCI_TC_DUMP_LEVEL);
    DCI_Capture_ReleaseBuffer(f.index);
#endif

    DCI_Capture_Stop();
    return TC_PASS;
}
```

其余三个用例的差异只在第 2/5/6 步的参数和第 8 步的断言：

- **RGB 连续**：第 6 步传 `CONTINUOUS`；第 7~9 步进 `for (i < DCI_TC_FRAMES)` 循环；第 8 步改为断言 `seq` 连号（`seq[i] == seq[i-1] + 1`）与 `index` 按 `buf_count` 轮转，并**校验相邻帧内容不同**（抓「DMA 没重新装载、反复交同一帧」）；末尾 `Stop()`。
- **JPEG 快照**：第 2 步传 `CAMERA_FMT_JPEG`，第 5 步传 `jpeg = true`；第 8 步没有期望长度，改为 `0 < bytes <= buf_bytes` + `tc_check_jpeg_soi_eoi()`（首两字节 `FF D8`、末尾 `FF D9` 后允许 0~3 个补零字节，直接验证 TESTPLAN 的 9.2~9.4）。
- **JPEG 连续**：JPEG 快照 + 循环 + 断言**帧长逐帧变化**（JPEG 是变长的，若每帧长度完全相同，极可能是没在重新采集）。

---

## 6. framebuffer 管理

```c
/* tc_fb.c */
#define TC_GUARD_WORDS   4
#define TC_GUARD_VALUE   0xA5A5A5A5UL

/* 每块 buffer 前后各夹哨兵。整体 4 字节对齐（DMA 按 word 写，DESIGN.md §10.4） */
typedef struct {
    uint32_t guard_lo[TC_GUARD_WORDS];
    uint32_t data[DCI_TC_BUF_BYTES / 4];
    uint32_t guard_hi[TC_GUARD_WORDS];
} tc_fb_t;

TC_FB_SECTION static tc_fb_t s_fb[DCI_TC_BUF_COUNT];

uint32_t *tc_fb_get(uint8_t i) { return s_fb[i].data; }
```

四条要点：

| 项 | 处理 |
|---|---|
| **对齐** | `uint32_t` 数组自带 4 字节对齐，即 `DCI_MAX` 的唯一硬要求（DESIGN.md §10.4） |
| **哨兵** | 前后各 4 个 `0xA5A5A5A5`，**每次 capture 结束都查**（TESTPLAN 13.5）。这是唯一能廉价抓到「描述符 `DAR`/`BLOCK_TS` 算错」的手段 —— 分块算错的第一症状就是越界写 |
| **cache** | 驱动不做任何 cache 维护、也不校验内存区（DESIGN.md §10.1），**是本工程的责任**。CPU 读 buffer 做 CRC/dump 之前必须 invalidate，否则读到 stale line（看到旧帧或新旧混合）。仓库先例：`bindings/honeygui/hardware_engine/PPE/AmebaG2/acc_ppe.c:511` 的 `DCache_CleanInvalidate()`。最省事是把 `TC_FB_SECTION` 指向非缓存区，一次维护都不用做 |
| **capture 前清零** | 填 `0xDEADBEEF` 而非 0 —— 这样「一个字节都没搬进来」和「搬进来的确实是 0」可以区分，`tc_check_not_flat()` 才有意义 |

---

## 7. 完成判定与自动校验

裸机 + `POLL` 模式下的取帧：

```c
tc_result_t tc_step_wait_frame(DCI_frame_t *f, uint32_t timeout_ms)
{
    DCI_ERROR e = DCI_Capture_WaitFrame(f, timeout_ms);   /* busy-wait，无 OS 依赖 */
    if (e == DCI_ERROR_TIMEOUT) { return TC_FAIL_TIMEOUT; }
    if (e != DCI_SUCCESS)       { return TC_FAIL_START; }
    tc_fb_invalidate(f->index);                            /* CPU 读之前 */
    return TC_PASS;
}
```

> `WaitFrame()` **不可在 ISR 调用**（自旋）；`CALLBACK` 模式下改为回调里只记录帧描述符、主循环再 dump —— **绝不在 ISR 里 dump**，UART 打印几百 KB 会让后续帧全部溢出。

**场景无关的自动校验**（真 sensor 对着实景，没有 golden 数据可比，但以下检查与拍到什么无关）：

| 检查 | 能抓到的问题 |
|---|---|
| `bytes == frame_bytes`（定长） | 几何声明错、分块算错、crop 静默截断 |
| `!truncated` | 帧偏长/偏短、JPEG 越界 |
| 哨兵未变 | 描述符 `DAR`/`BLOCK_TS` 越界写 |
| 非全 `0xDEADBEEF`、非全同值 | 完全没搬到数据、数据总线卡死 |
| 相邻帧内容不同（连续模式） | DMA 没重新装载、反复交付同一帧 |
| `seq` 连号 | 交付了但没被取走（`OVERWRITE` 覆盖 / POLL 队列顶掉） |
| `index` 按 `buf_count` 轮转 | 双缓冲切换错 |
| JPEG `FF D8` 开头、`FF D9` + 0~3 补零结尾 | JPEG 通路与补零行为（TESTPLAN 9.1~9.4） |
| `DCI_GetStats()` 除 `frames_done` 外全 0 | `OVR`/`ERR`/`resync`/`suspend`/`buf_overflow` 任何一项非 0 都说明有隐患 |
| CRC32 记录 | 跨版本回归比对；同一场景两次运行 CRC 应一致（静止画面） |

> **`tc_check_stats_clean()` 是性价比最高的一条断言。** 图像内容对不对肉眼还能看，但 `suspend_count` 悄悄涨了 200 次这种事只有查统计才知道。

**若 sensor 支持内建测试图案**（多数 sensor 有色条/渐变，`DCI_TC_SENSOR_TESTPAT = 1` 开启），则可做**像素级精确校验** —— 色条边界应落在已知列号上。这是真 sensor 环境下唯一能做像素级判定的手段，强烈建议 camera 驱动把它实现出来。

---

## 8. Dump 分级

466×466 RGB565 = 434 KB，全量 hex ≈ 870 KB ≈ 7 s @1 Mbps。连续模式采 8 帧就是 1 分钟 —— 必须分级：

| 级别 | 内容 | 耗时 | 用途 |
|---|---|---|---|
| **L0** | 摘要：`bytes`/`seq`/`index`/`truncated` + CRC32 + 全量 stats | < 10 ms | 连续模式、长压力 |
| **L1** | L0 + 采样点：四角、中心、每行首像素 | ~100 ms | **默认**。够判断「图像结构对不对」（行首规律断了就是 stride/分块错） |
| **L2** | L0 + 全量 hex（带地址前缀，PC 脚本可还原成图） | 秒级 | 单张快照、疑难排查 |
| **L3** | L2 + 写外部 flash/SD（若有） | — | JPEG 存 `.jpg` 直接用看图软件打开验证 |

dump 头部一行带全部元信息，便于 PC 侧脚本解析与回归比对：

```
[DCI-DUMP] case=RGB565/SNAPSHOT w=320 h=240 fmt=RGB565 bpp=2
           bytes=153600 expect=153600 seq=1 index=0 trunc=0 crc32=0x8A3F21C7
[DCI-STAT] done=1 drop=0 chunks=1 ovr=0 err=0 resync=0 suspend=0 ovf=0
```

**JPEG 用例应默认 L3（或 L2）** —— JPEG 帧通常只有几十 KB，且能直接落地成 `.jpg` 用看图软件打开，这是比任何断言都直观的验证。

**失败时自动 `DCI_RegDump()`**：出错那一刻的寄存器状态才有诊断价值。`DCI_RegSnapshot()` 是 ISR 安全的，可以在 ISR 里抓、回主循环再打印（DESIGN.md §13.1）。注意 reg dump 由 `DCI_DEBUG_REG_DUMP` 门控，测试工程应当**始终打开**这个宏。

---

## 9. Bring-up 阶梯

依赖关系决定顺序，且**前三级不需要 DMA，今天就能跑**（GDMA 握手索引还是 `0xFF` 占位，不阻塞这三级）：

| 级 | 做什么 | 需要 DMA？ | 通过标志 | 能排除的问题 |
|---|---|---|---|---|
| **L0** | `DCI_RegTestDump()` —— 寄存器读写自检（DESIGN.md §13.6） | ❌ | `verdict: PASS` | **DCI 时钟未开、基址错、总线死、位域定义与硬件不符**。连 sensor 都不用接 |
| **L1** | 只跑 `tc_step_camera_init()`，I2C 读回 chip ID | ❌ | ID 匹配 | 供电、IO 复用、I2C 接线与时序 |
| **L2** | `tc_step_probe_signal()`：数 VSYNC/LINE | ❌ | 测得行数 == `CAM_OUT_HEIGHT` | PIXCLK/VSYNC/HSYNC 接线与极性、**sensor 实际输出尺寸**（§4.2 的 C3） |
| **L3** | RGB 快照，最小分辨率（如 160×120） | ✅ | 帧长精确、哨兵未变 | DCI 数据通路、GDMA 握手、描述符构建 |
| **L4** | RGB 快照，目标分辨率 | ✅ | 同上 | 带宽、FIFO 余量 |
| **L5** | RGB 连续，`buf_count=2` | ✅ | `seq` 连号、相邻帧不同 | `evt` 会合、闭环拓扑、门控背压 |
| **L6** | JPEG 快照 | ✅ | SOI/EOI 完整 | 变长通路、`CURR_DAR` 取长度 |
| **L7** | JPEG 连续 | ✅ | 每帧 SOI/EOI、帧长逐帧变化 | ISR 内自动重装、防御性关 `CAPTURE` |

**L0 必须第一个跑，而且是无条件的。** DCI 时钟未开时寄存器读回全 0，这个症状会把人引向查基址、查接线、查 sensor —— 而真正的原因在时钟树上（DESIGN.md §17 项 12：RTL87X3K 的 RCC 表里没有 display 子系统时钟项，很可能走 `DISPLAY_PCC_BASE`）。L0 一条日志就把它和「基址错」「总线死」一起判掉，成本几乎为零。它也顺带定论 `CAPCNT` 的编码（§17 项 11）。

**L2 是整条阶梯的关键。** 它把「集成阶段最常见的错误」提前暴露：宏声明 240 行、sensor 实际输出 480 行，在 L2 一行日志就说清了；若跳过 L2 直奔 L3，同样的问题表现为 `DCI_ERROR_FRAME_LONG` 或 `OVR`，得反推半天。

L3 用最小分辨率起步的理由：160×120×2 = 38 KB，双缓冲也只 76 KB，**排除内存与带宽这两个变量**，先把通路本身走通。

> **分块路径的覆盖缺口**：常见分辨率全是 1 块（`frame_words ≤ 1,048,575`），只有 2048×2048 才会走到 2/3 块。若板上内存装不下，把 `DCI_MAX_BLOCK_TS` 临时编译成小值（如 4096），320×240 就会被强制分块，从而用小分辨率覆盖多块组帧逻辑。分块是驱动里最容易错的部分，不该因为内存不够就不测（详见 TESTPLAN §15.4）。

---

## 10. 与既有代码的接缝

| 接缝 | 现状 | 测试工程要做的 |
|---|---|---|
| `camera_if.h` | 待新建 | 定义 `camera_init/set_mode/read_id/set_test_pattern`，测试工程只依赖它，换 sensor 不动测试代码 |
| `DCI_Handler` 中断挂接 | `rtl_dci_irq_init()` 目前被 `#ifdef DCI_VECTORn` 包住（`TODO(HAL)`） | HAL 到位前，测试工程可在 `tc_step_dci_init()` 后自行 `RamVectorTableUpdate` + `NVIC_Init` 挂 `DCI_Handler`，并在 README 记录为临时手段 |
| GDMA 通道 ISR | `rtl_dci_dma_irq_init()` 已可用（`RamVectorTableUpdate`） | 无需额外处理 |
| GDMA 握手索引 | **已解决 = 48**（AP DMA1）。见 DESIGN.md §17 项 1 | 不再阻塞。reg dump 里应看到 `SRC_PER=0x30`；**注意该字段是 4+3 位拼的**，只解码低 4 位会把 48 显示成 0 |
| `DCI_NONCACHE_SECTION` | 空定义（`TODO(HAL)`） | 描述符环是驱动自己的事；测试工程只需管好自己的 framebuffer（§6 的 cache 一行） |
| `DCI_DEBUG_REG_DUMP` | 默认 0 | 测试工程编译时**强制置 1** |
| Kconfig / CMake | DCI 的第 13 步尚未做 | 新增 `CONFIG_REALTEK_DISPLAY_ENABLE_DCI` 与 `CONFIG_REALTEK_CAMERA_<PART>`；测试工程用 `CONFIG_REALTEK_DCI_TEST` 单独门控，默认 `n`，不影响正式构建 |

---

## 11. 待决事项

| # | 事项 | 影响 | 建议 |
|---|---|---|---|
| 1 | **具体 sensor 型号未定** | 寄存器表、chip ID、支持的模式、是否有内建测试图案 | 先按 `camera_if.h` 接口占位，`camera_<part>.c` 待硬件确定后填。测试工程不受影响 |
| 2 | **sensor 是否支持 JPEG 直出** | 若不支持，JPEG 两项测不了 | 尽早确认。若不支持，JPEG 通路只能靠 FPGA 激励（TESTPLAN §15.2）验证 |
| 3 | **RGB565 的总线宽度与 pclk/pixel** | §4.1 中 `PCLK_PER_PIXEL = 2` 的前提 | 确认接的是 8-bit 还是 10-bit，两个宏有耦合 |
| 4 | **framebuffer 放哪个内存区、是否可缓存** | 决定 §6 要不要做 cache 维护 | 优先放非缓存区，一次维护都不用做 |
| 5 | **UART 波特率与 dump 落地手段** | 决定 dump 级别的实际可用性 | 若有 flash/SD，JPEG 用例强烈建议做 L3（直接存 `.jpg`） |
| 6 | `CAPCNT` 编码（N-1 还是强制清低 2 bit） | 只影响 crop，本工程四项都不开 crop | 不阻塞本工程；见 DESIGN.md §17 项 11、TESTPLAN §14 项 5 |
