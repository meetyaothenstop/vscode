# MAIN RAW1：150kHz整数快速路径

2026-11-03。用户同意精简快速路径；PWM/ADC/软件保护150kHz，电流50kHz、电压10kHz及T1..T7保持。
修改前040164c已以`pre-fastirq-raw-20261103`备份到GitHub，并核对标签解引用SHA。
仍使用`MDK-ARM/bidreaction.uvprojx`，唯一台架参考不变；用户自行下载。
首页`MAIN ADC RAW1 50/10`，参数页`M-RAW1`。

## 现场与判断边界

用户Watch截图记录：T7 elapsed0xD6B=3435周期（22.90us）、spacing0x3E8=1000周期、phase0x3E60=15968ticks。
control_steps0xAB=171、voltage_steps0x23=35、完成的请求响应峰值0xB57=2903周期（19.35us）。
故障左0.596145272V、右11.9357195V、电流-0.101270676A；参考0.50800091V。
isr_cycles_max0x301=769周期是历史检查点峰值，可能含T7故障锁存，不能说正常运行始终占77%CPU。
这些数据证明worker运行过且本次未按时发布，尚不能单独确定纯PI耗时、持续中断占用或最终电气根因。

## 改动

- 前台停止状态预先枚举4096个ADC码，依据原浮点标定和原比较式建立整数接受区间。保留输入/OVP、绝对过流、ADC近轨、方向16样本、启动及HOLD恢复的严格/包含边界。
- 150kHz路径保存新鲜原始样本并做整数判断；每50kHz请求携带自己的原始采样快照，worker再换算和计算。停止/故障遥测由前台刷新，运行遥测由worker刷新。
- 启动接受也直接快照最新原始值；第一故障锁存对故障当次完整原始样本换算，不能误用上个worker的浮点遥测。
- 最小/空脉冲调制由worker预计算Q24增量，快中断仅整数累加/进位/成对比较值写入；原最小脉宽和平均指令保留至Q24量化精度。
- 浮点故障换算明确放在不内联的冷路径。占空比/参考遥测在快中断按位复制，避免VLDR/VSTR也触发浮点上下文保存。匿名联合保持原字段及布局。
- 30+2周期自举/装载、至少3周期Boost全关断、STOP/零目标/故障取消、两侧同步预装载均保持；原保护门限及T7没有放宽。

## 新增Watch字段

| 字段 | 范围及限制 |
| --- | --- |
| fast_run_cycles_max | 无故障活动快速路径到收尾检查点的峰值，排除本次已锁存故障的路径；不包含完整异常进出 |
| control_start_delay_max | 请求开始到worker入口的最大等待 |
| control_run_wall_cycles_max | worker入口到完成检查点的墙钟周期，包含抢占，不是纯PI执行周期 |
| control_fast_c_cycles_max | worker期间累计观测的快速IRQ C检查点耗时，不包含全部异常进出、早退路径或其他IRQ |
| control_cycles_max | 保留原请求开始到worker完成检查点的响应峰值 |

各字段峰值可能来自不同周期，不能直接相减拼成一次完整时序。150MHz下周期数除150得到us。
所有worker时间峰值只在完成路径更新；T7的fault_elapsed仍用于未完成的请求。

## 验证与交接

逐ADC码等价、启动新鲜度、故障新鲜锁存和现有四模式/13工况开关模型均测试真实控制器及后台。
ARM实际指令检查覆盖Power_TimerIRQ、raw_checks、control_slice、control_modulate、control_write；
并覆盖raw_can_resume、enable_outputs、PWM_EntryIsTimely、PowerProtection_FaultPending及链接器跳板；
begin_bootstrap若独立保留也检查，本次已内联进被检查的Power_TimerIRQ。
旧040164c产物检查失败（包含FP指令），最终RAW1产物检查通过（这些函数中无FP指令）。
故障冷路径仍可做浮点换算，这不是“整个工程没有浮点”。

独立只读审查发现4项P2：启动有效性与原始采样未同锁、拒绝启动的首故障可能混用ADC相位、
取消的worker可能继续发布统计/遥测、墙钟与快IRQ累计采样端点不同步。前三项先观察到失败再修正通过；
时钟/累计端点改为同一短临界区，最终发布也在短临界区复核代号与pending后一次完成。
修正后由作者复测，未宣称二次独立审查。保持非RAW台架原实现，避免无关编译变化。

最终62组回归全部通过（GCC C11/O2/Wall/Wextra/Werror，含实际四模式后台、13个名义开关工况、
4096码逐点保护等价、启动快照/拒绝故障相位/发布前故障取消、Q24边界进位和双向脉冲/停机）。
ARM6.18两个既有工程均重建0错误0警告。主Code48164/RO1320/RW44/ZI5780；
CCM9688字节，执行0x10000000、加载0x08009B74，主栈2048字节；台架栈1024字节。
实际HEX启动拷贝/HRTIM/PendSV向量、worker及快速代码CCM、Flash紧急关断、OLED DMA/SRAM和AXF导出字节一致性均通过。

- 主HEX SHA256：`EC70E24DD17AC423BAA0C875F83C4F595246AE9813FD9F72FBE6F1239F5CA04A`。
- 主AXF SHA256：`16836826F5B92C5DE67BA33F8AC79FE832830903AF7B206138C6DDF37B82E720`。
- 台架HEX SHA256：`A9BA257BA3FFBB49C8FAED9803E3462671F692A9B4D063C9975512D09EF6D451`，与修改前逐字节一致。

版本标签`main-fastirq-raw1-20261103`，机器记录见`控制降频RAW1-验证.json`。
构建产物仍在原MDK-ARM目录，不将可重建的HEX/AXF或IDE界面状态提交Git。
未下载/操作板子，不能宣称T7或实板稳定性已修复，未验证硬件OCP接线。
下载后加载本版配套AXF，确认首页。若再T7，保留同一现场，读取原fault elapsed/spacing/phase及上述新字段。
