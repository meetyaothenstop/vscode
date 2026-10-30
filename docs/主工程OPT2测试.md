# 主工程OPT2：内联比较值映射候选 — 2026-10-30

工程仍为`MDK-ARM/bidreaction.uvprojx`，Target `bidreaction`，
固件`MDK-ARM/bidreaction/bidreaction.hex`。不新增工程。
首页`MAIN ADC OPT2 6A`，参数页后缀`M-OPT2`，`POWER_UNIFIED_DIAGNOSTICS=0`。

最新用户截图：OPT2交付后仍报64 T4，已取得本次792周期/间隔1000/27232刻度，
尚未独立核验运行时固件身份。下表806周期/27680刻度属于前一OPT版，不能当作OPT2数据。完整接续见
[对话接续摘要](对话接续摘要-2026-10-30.md)。

## OPT2交付后的新故障截图

来源：用户上传`codex-clipboard-c7cfd01e-6a3c-4d22-a2a8-3921174e4ad0.png`。
`&power_status=0x20000B08`与当前OPT2 MAP一致；本轮只读取本地源码/映射和用户截图，
未连接、运行、暂停、复位、下载或清故障，未取得板上Flash指纹/首页身份。

| 字段 | 截图值 |
| --- | --- |
| state / fault / detail | POWER_FAULT / 0x40=64 / 4 |
| fault_elapsed_cycles | 0x318 = 792 |
| fault_spacing_cycles | 0x3E8 = 1000 |
| fault_counter_ticks | 0x6A60 = 27232 |
| isr_cycles_max / v_reference | 0 / 0.10000001V |
| fault_v_left / fault_v_right / fault_il | 23.9685459V / 0.173232496V / 0.100805283A |

792低于840，但27232高于26880共352刻度；在150MHz/32倍高分辨计数配置下，
等效11个CPU周期、约73.33ns。写前相位约5.673us，门限为5.600us。
本次与旧OPT截图的写前耗时和相位均相差14个CPU周期；不同次启动、身份未独立核验，
不能将其当作严格A/B测速、最坏余量或T4修复证据。
两组`counter/32 - elapsed`均约59周期，包含入口、前导和读取时差等影响，
尚不能将全部差值归因于FPU或某一函数。参考约0.1V且isr_max为0与首次控制步写前故障一致，
不是零耗时，也不是输出已经持续稳压。截图上方实时采样继续变化，分析首故障应使用`fault_*`。

下一步重新核算完整中断与首次使能路径的最坏预算；不能以只省11周期为余量目标，
也不直接引入RTOS或放宽门限。多速率外环等结构方案仍待评估，当前未改控制代码。

## 前一OPT故障依据（历史）

用户下载前一OPT版后报告64 T4。助手只在既有调试会话添加/展开`power_status`并滚动，
未发送连接、运行、暂停、清故障、复位或下载命令。读到：

| 字段 | 锁存值 |
| --- | --- |
| fault/detail | 64 / 4 |
| fault_elapsed_cycles | 0x326 = 806 |
| fault_spacing_cycles | 0x3E8 = 1000 |
| fault_counter_ticks | 0x6C20 = 27680 |
| v_reference | 0.10000001V |
| fault_v_left/right/il | 24.0211468V / 0.0692929998V / 0.100805283A |

耗时已低于840周期，但写入相位超过26880刻度800刻度，等效25个CPU周期。
与前一次858周期的不同次启动相比，本次读数少52周期；这不是严格受控提速或最坏余量证明。
isr_cycles_max为0，参考约0.1V，与第一个控制步写入之前故障一致。
主程序中的精确T4条件为写前耗时未超过840而实际计数器>26880。

## OPT2改变了什么

将同一份前向PWM算术移入私有内联头文件；通用映射仍调用这份实现，
主Buck用只产生左右比较值的内联适配器，避免外部调用和未使用占空比字段的保存。
HOLD恢复仅保留硬件实际使用的两个比较值，原门极使能、预装载和恢复顺序保持。
完整`PwmPlan`映射输出继续用于通用API/等价回归，类型布局不变。

映射算术与零脉冲策略保持原样，PI/参考限幅/软启动、四模式、ADC相位与时钟不变。
6A参考、7.5A采样停机、840/936耗时保护和26880写入相位门限不变。
OPT2仍是候选，尚未证明实板节省多少周期或消除T4。

## 软件验证与身份

46组主机回归通过，含HOLD/恢复、四模式、故障竞态和DMA；
200016条映射指令对照覆盖完整/比较值适配器与通用映射，并检查非法指令/空指针不修改输出。
镜像检查在旧OPT版因仍调用独立Buck映射而失败；OPT2实际ARM/MAP确认已无该调用。
反汇编同时确认Buck路径不再保存两个未使用的浮点占空比字段。
这些是生成代码证据，不是实际周期或电气验证。

主ARM6.18零错误零警告：Code45588/RO1288/RW44/ZI4500；
CCM7756字节，运行0x10000000、加载0x080098D0。
实际HEX的CCM启动拷贝/向量/四模式热函数/无探针/DMA SRAM通过；AXF导出的HEX完全一致。
主HEX SHA256：`D845A25DB8D14F24294885701FA8EED4EE5DF230A169D3FF4DE6DA3EC2BF93E8`。
共享源码台架重建零错误零警告，CCM3376字节；其新HEX指纹为
`A9BA257BA3FFBB49C8FAED9803E3462671F692A9B4D063C9975512D09EF6D451`，未进行该台架映像板测。

## 用户自行下载及反馈

先保存前一版故障数据，保持输出关闭，由用户自行下载OPT2，
核对首页新标识，再按原24V输入/12V目标/低功率负载复现一次。
Keil当前配置仍应保持`POWER_UNIFIED_DIAGNOSTICS=0`，不必切回DIAG。

若仍故障，保留本次记录，Watch使用`power_status`，读取
`fault`、`fault_detail`、`fault_elapsed_cycles`、`fault_spacing_cycles`、
`fault_counter_ticks`、`isr_cycles_max`和`v_reference`。
本版没有`power_diag`；其cannot evaluate不代表ADC故障。
若启动成功，记录万用表输出、OLED读数，并继续测量最坏IRQ与写入相位。
本轮全范围/满载/硬件OCP和OPT2实板时序均未验证。
