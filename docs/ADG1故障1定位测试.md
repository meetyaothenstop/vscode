# MAIN ADG1：启动时偶发报1的定位

2026-11-04。用户确认FRM1的OLED可以正常刷新；本次偶发报1出现在
按START一开始，仍为右侧12V输入、左侧6V输出、100Ω负载。
报1是ADC有效性保护，本次还没有能区分其触发分支的实板记录。
它不直接证明电流环频率过高，也不是报16的电流方向判据。

## 本版内容

原主工程`MDK-ARM/bidreaction.uvprojx`，Target `bidreaction`。
首页`MAIN ADC ADG1 50/10`，参数页`M-ADG1`，`power_event.version=3`。
保留FRM1界面续传。PWM/ADC/快速保护150kHz、电流环50kHz、电压环10kHz，
PI、保护条件与阈值不变，保留正向Buck/Boost、反向Buck/Boost。

只在原来的ADC故障分支增加诊断：先关闭输出，然后在同一个首故障锁内
保存中断入口已经读取的ADC1、ADC2标志，最后发布完整故障包。
不使用关断后重新读取的标志；正常快速IRQ/worker不新增记录调用。
停机、前台刷新、后续中断和未满足条件的清错不会覆盖首个故障包。
成功清错才清包。报ADC故障时电压/电流快照仍是上一次完整样本，
不应当作当前无效转换的有效测量。

`POWER_ADC_FAULT_DETAILS`默认跟随MAIN事件诊断；关闭时保留旧ADC故障detail=0。
台架默认关闭，仍是唯一`bench12_buck_fast`参考工程。
若自行关闭`POWER_OLED_FRAME_RESUME`而保留ADC诊断，首页会沿用EVT1标识，
应以事件版本3和新增ADC字段识别；本次发布的主工程两项均开启，显示ADG1。

## 用户复测

由用户下载`MDK-ARM/bidreaction/bidreaction.hex`，加载同次构建的
`MDK-ARM/bidreaction/bidreaction.axf`。确认首页ADG1；不要沿用旧版本地址。
复现报1后先保留现场，不按清错或复位；在Keil Watch展开：

- `power_status`：特别是`fault`、`fault_detail`。
- `power_event.fault`：`valid`、`code`、`state`、`mode`、`start_pending`，
  以及新增的`adc_reason`、`adc1_entry_flags`、`adc2_entry_flags`。

以下数值是`adc_reason`（同次首故障时也写入`fault_detail`），不是OLED故障码：

| adc_reason | 原ADC保护触发条件 |
| --- | --- |
| 1 | ADC1入口JEOS缺失，未观察到注入序列完成标志 |
| 2 | ADC1入口JQOVF置位，注入队列溢出标志 |
| 3 | 上面两项同时成立 |
| 4 | 需要ADC2延后样本的正向Buck中，EOC/EOS不齐或OVR置位 |

ADC1检查优先；若两路同时异常，先记录ADC1分支，但两路原始标志都保存。
反向模式仍使用ADC1原相位，不会因为未使用的ADC2样本无效而报1。
非ADC首故障的三个新增字段均为0。仅停止时的无效采样不新增报错。
中断清标志后，直接看活的`ADC1->ISR`/`ADC2->ISR`不能还原入口状态。

这份记录用于确定下一步检查转换触发、清标志或采样相位的方向；
尚不能仅凭JEOS缺失确认ADC速度不足。正常场景ADC时序和板上最坏执行时间
仍需实板测量；主机测试不验证硬件OCP接线或就绪状态。

## 验证与历史

修改前源码`d54d61aee9951aa03fead36cf227ca5c86516ca1`已用
`pre-adcdiag-20261104`上传GitHub并核对远端解引用SHA。
主机真实控制后端覆盖待启动、自举、运行三个阶段，ADC1两种条件及其组合，
ADC2四种缺失/溢出组合，反向忽略未用ADC2、首包冻结、成功/失败清错和
非ADC故障不继承旧标志；另验证关闭新增诊断及关闭整个事件诊断的后端。
完整结果、镜像指纹及检查范围见[验证记录](ADG1-验证.json)。
独立只读代码审查无Critical/Important问题；自定义关闭界面续传时的版本标识
差异为Minor，已在上文说明，独立选择标签的代码整理留待后续。
助手仅准备/编译/验证固件，由用户操作下载；未操作或测量板子。

## ADG1首次实板故障截图

用户截图锁存字段：valid=1、code=1、detail=1、state=1（BOOTSTRAP）、
mode=3（反向Buck）、run_id=1、start_pending=0、bootstrap_resume=0、
bootstrap_remaining=0x1E（30）、arming_remaining=0、resume_pending=0。
目标6V，duty/reference_volts/两侧compare记录均0。
run_age_cycles=0x570（1392），按固件150MHz约9.28μs；run_age_ticks=0。
总样本数为0x5DF52，截图未显示尾部ADC入口标志。

state是关断前锁存的阶段，不是读取截图时的实时状态。根据代码，已开始
冷启动自举且尚未递减一个完整自举周期，正常PI闭环尚未投入运行。
detail=1确定本次触发ADC1入口JEOS缺失分支；还不能确定是触发、转换完成
还是清标志时序导致。后续需补adc1_entry_flags、adc2_entry_flags、adc_reason。
这次截图不验证正常运行时的最坏执行时间或物理根因，不修改频率/保护策略。

随后补充截图：adc1_entry_flags=0x1、adc2_entry_flags=0xF、adc_reason=1。
用户确认持续运行、只按板上START，没有暂停/单步。ADC1只有ADRDY，缺JEOS；
ADC2有EOC/EOS且无OVR。后续入口时序观测见[ADG2说明](ADG2启动ADC时序测试.md)。
