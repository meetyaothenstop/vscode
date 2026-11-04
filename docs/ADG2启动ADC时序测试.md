# MAIN ADG2：区分ADC未完成与前一次IRQ迟到

2026-11-04。ADG1实板：右12V→左6V、100Ω，用户确认上电持续运行，只按
板上START，没有暂停/单步。故障包valid=1、code=1、detail=1、state=1
（冷BOOTSTRAP）、mode=3、自举剩余30；启动年龄1392 CPU周期约9.28μs。
补充截图：adc1_entry_flags=0x1、adc2_entry_flags=0xF、adc_reason=1。

## 已确定和待验证的部分

ADC1的0x1只有ADRDY；JEOC、JEOS、JQOVF均0。ADC2的0xF包括ADRDY、EOSMP、
EOC、EOS，OVR为0。ADC1没有序列完成标志是报1的直接条件，ADC2本次有
完成标志。标志为0也可能是事件已被软件清除，不能据此断定没有收到触发。
定义见[ST RM0440，ADC_ISR及21.4.23/24节](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)。

正常闭环PI尚未投入运行。这不是本次PI执行耗时的证据；启动仍受ADC/PWM
时序约束。名义上ADC1在周期25%触发，三次转换约2.96μs完成。

构造的真实后端实验能产生相同故障：前一次STOP/待启动IRQ延迟至周期内
CPU相位750（HRTIM计数24000），读走本周期已完成的ADC1并清JEOS、开始自举；
下一正常IRQ在相位20到来，仅相隔270周期，此时尚无更新的ADC1完成标志。
ADC2的延后转换此时已可完成，因而记录1/F、reason1和BOOTSTRAP剩余30。
测试另有正常相位对照：连续正常周期不会报ADC故障。

这是手动输入的合成时序，**不证明实板真的迟到**。新增测试用真实控制
后端和真实ADC_ISR掩码；ADC标志/时间由夹具注入，没有测量实体转换。
测试为`tests/adc_bootstrap_phase_test.c`，预包含`tests/adc_real_isr_flags.h`，
与其他组使用的合成标志位相互隔离。实验运行包含在`tests/run_tests.ps1`。

链接反汇编还确认，启动提交的PRIMASK锁内会清零/复制两份172字节事件
邮箱。此处为待测的延迟来源之一；持续时间、是否导致本次故障均未证实。
本版不把这些操作移出锁，也不修改启动资格、保护条件、门限、PI或频率。

## ADG2新增记录

原主工程、Target `bidreaction`。首页`MAIN ADC ADG2 50/10`，参数`M-ADG2`，
`power_event.version=4`。事件结构布局保持；版本4表示ADC故障还填写原有
PowerStatus时序字段。三个ADC入口字段继续保留。
本次发布同时开启界面续传与ADC诊断；自定义关闭界面续传而保留ADC诊断
时，首页仍沿用EVT1标签，应以事件版本4及新增时序语义识别。

只在原ADC故障分支传入已经读取的IRQ开始时间、入口间隔和计数器；
先关输出，再在原首故障锁中建包。没有新增正常IRQ记录调用或硬件读探针。
`fault_elapsed_cycles`在关断之后、复制故障包之前采样，包含冷故障分支开销，
不能当成健康IRQ运行时长。`fault_spacing_cycles`和`fault_counter_ticks`
是原入口读数，不是关断后重新读取的计数器。首故障冻结/安全清错沿用。
关闭ADC诊断仍保留旧detail/时序字段为0；受限bench固件指纹保持。

## 用户下一次复测

由用户下载`MDK-ARM/bidreaction/bidreaction.hex`，调试加载同次
`MDK-ARM/bidreaction/bidreaction.axf`。确认ADG2。保持原工况持续运行，
只按板上START，报1后先保留现场。Watch直接添加：

```text
power_status.fault
power_status.fault_detail
power_status.fault_spacing_cycles
power_status.fault_counter_ticks
power_status.fault_elapsed_cycles
power_event.fault.adc1_entry_flags
power_event.fault.adc2_entry_flags
power_event.fault.adc_reason
```

正常周期为1000 CPU时钟（6.667μs）。若ADC故障的入口间隔明显偏短
（实验为270），且当前计数器在周期前段，支持前一次IRQ迟到并提前消耗
采样的假设；若接近正常周期，则需继续检查触发/ADC状态及标志清除。
计数器和DWT入口读数不是同一条指令时刻，不做无误差的相位反算。
不根据一次标志记录直接降低电流环或电压环频率。

## 验证与历史

修改前7564102已用`pre-adcphase-20261104`上传并核对远端解引用SHA。
71组主机回归及诊断关闭兼容检查，主/bench ARM构建、实际镜像/CCM/栈/DMA/
AXF与快速整数指令检查的结果见[验证记录](ADG2-验证.json)。
独立只读审查完成，无Critical/Important/Minor问题；结论仅支持诊断版交付。
保留FRM1 OLED续传、四种模式和唯一受限bench；PWM/ADC/快速保护150kHz、
电流50kHz、电压10kHz。由用户下载，助手没有操作板子；未证明实板报1/16
消除、最坏执行时间、电气行为或硬件OCP接线。
