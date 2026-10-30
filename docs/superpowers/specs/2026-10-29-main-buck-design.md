# bidreaction主工程集成设计

用户要求将已修改代码用于既有`MDK-ARM/bidreaction.uvprojx`，Buck输入标称12–40V、
输出菜单0–36V、限流控制6A。用户明确保留升压和反向功能，并选择沿用当前ADC软件保护测试。
用户自行下载；本任务不连接板子、烧录、复位或启动功率级。软件验证不宣称板上全范围通过。

## 方案与范围

主工程新增`POWER_MAIN_APPLICATION=1`和显式`POWER_MAIN_ADC_SOFTWARE_ONLY=1`，
bench配置均为0，不借用bench标志绕过主工程功能。继续既有主工程/Target和唯一bench参考，
不复制工程。相较直接启用FAST bench，这保留模式2/3/4；相较继续Flash/阻塞OLED，
迁移CCM、优化Buck、前台DMA显示与晚采样能力。旧32组配置行为保持。

- 主工程四模式均保留；正向Buck走优化步，正向Boost走完整步，反向原算法保留。
  一个私有编译期步模板生成fast/full函数，避免维护两份方程；切换经STOP/初始化。
- 电流参考上限6A，采样过流保留原主工程7.5A，为跟踪瞬态留余量；与旧台架1A参考/6A保护区分。
  硬件OCP状态不伪造；ADC-only授权只绕过就绪启动限制，仍保留故障源/ADC/时序关断。
- 主工程Buck（模式1/3）输入菜单12–40V、输出0–36V，菜单保持1V步进；非零后端目标1–36V。
  0和-0是显式OFF请求：先于样本/输入/就绪/当前运行态检查关闭全部四路输出、取消待启动，
  保留FAULT和首故障记录；不是主动放电或0V闭环，电容由负载放电。
  mailbox编号保持，target=0后提交START_BUCK(4)/START_REVERSE_BUCK(5)才能执行；单改target不生效。
- 正向Buck最大Dcmd0.95：请求及启动/运行期间检查目标<=0.95×实际输入；不可达报启动/输入故障，
  不自动升压替代该模式。反向Buck保留原严格输出<实际输入，正反Boost原范围与算法保留。
- Buck实测输入保护10–42V，为标称12/40端点留误差余量；Buck过压目标×1.25（36V目标45V），
  冷启动|IL|<0.5A/输出<=1V；其他模式沿用原6–40V、1.1倍OVP、0.25A/0.25V冷启动。
- DMA显示能力默认主工程开启，200ms一帧，键/STOP优先；待启动/准备阶段静默，RUNNING更新。
  沿用独立缓冲、低优先级IRQ和错误离线策略，不从回调链发包。
- ADC2同PA0晚采样只用于正向Buck；STOP阶段提供Buck新鲜输入。Boost/反向活动反馈保持ADC1，
  每周期消耗ADC2标志以防陈旧；需要ADC2的上下文坏样本仍关断。
- 新主工程标识：主界面`MAIN ADC SW 6A`，参数页` SW-MAIN`。原台架身份/范围不变。
- CCM覆盖HRTIM封装/控制热路径、dispatcher、fast/full forward、reverse step/hold/resume；
  早期启动/紧急关断仍Flash，DMA/数据/栈普通SRAM。840/936门限保持，板上最坏耗时待验证。

## 实现接口

Core/Inc/power_build_config.h由root管理：
`POWER_MAIN_APPLICATION`/`POWER_MAIN_ADC_SOFTWARE_ONLY`默认0；
`POWER_OLED_DMA=(MAIN||BENCH_LIVE_OLED)`、`POWER_LATE_INPUT_ADC=(MAIN||BENCH_LATE_INPUT_ADC)`、
`POWER_FORWARD_NULL_DUTY=(MAIN||BENCH_12V_BUCK_TEST)`；CCM默认MAIN，允许主工程。
主Buck常量：`POWER_MAIN_BUCK_INPUT_MIN_VOLTS=10`、`MAX=42`、`MAX_TARGET_VOLTS=36`、
`OUTPUT_OVERVOLTAGE_RATIO=1.25`、`START_CURRENT_LIMIT_AMPS=.5`、`START_OUTPUT_MAX_VOLTS=1`、
`MAX_DUTY=.95`。主菜单`POWER_MAIN_INPUT_SETTING_MIN=12U`、`MAX=40U`，默认正向24→12，反向36→12。
保留通用宏作为其他模式原值（参考6/过流7.5/输入6–40/目标1–36/OVP1.1/冷.25/.25）。

## 验证与交付

保留32组旧回归，新增主工程四模式/切换/0V所有阶段、非法值、范围/不可达/输入跌落、
6A参考与7.5A采样保护独立边界、ADC路径/失败、DMA启动静默、菜单0/36/12/40和STOP。
fast/full轨迹比对覆盖12/24/40V、最高36V及6A参考。名义开关模型覆盖可达Buck运行点，
不把12V→36V当Buck测试。重建主/bench，验证实际HEX CCM拷贝/IRQ/所有模式函数/SRAM。
记录未验证的物理精度/全部负载/最坏IRQ；提交、标签、推送核对远程SHA。
