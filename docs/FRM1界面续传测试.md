# FRM1界面续传与分钟级观测

2026-11-04。用户补充：START后OLED不更新，万用表输出约5.9V，电流检测输出2.49V（已纠正先前2.45V），
约1～2分钟后界面刷新并报16，关断后检测输出仍约2.49V。按当前软件零点2.49V相当于接近0A的万用表读数，
不排除ADC采样点的短时越限。16要求连续16个采样点大于+1A，软件等效检测电压约2.59V。
本版不宣称已查明真实回流/模拟尖峰/ADC异常，也不改变PI、频率或任何保护门限。

## 版本与机制

改前14b4a76已用pre-oledframe-20261104上传并核对远端SHA。继续使用主工程MDK-ARM/bidreaction.uvprojx。
首页MAIN ADC FRM1 50/10，参数页M-FRM1。power_event.version=2，sample_hz=150000。

MAIN在瞬时STARTING/RUNNING状态变化间保留同一帧，完成8页后再合并显示新遥测；
按键、STOP、故障请求在正在传输的页面排空后优先重画。ScreenBuffer在同一帧期间保留，DMA仍使用不可变的独立页面快照。
页面只有数据包完成、前台消费完成标志后才前进；HAL_OK提交不再等于页面完成。
启动阶段不开始命令或数据包，已在传输的包仍可排空。尚未开始的数据包暂停保存，恢复允许时重置发送期限并继续；
刻意暂停不计入重试超时，但活动DMA和允许期间的HAL_BUSY提交重试仍保留原50ms保护。
恢复帧功能默认仅MAIN事件诊断版开启，历史无事件/台架路径保留原实现。

## 诊断读取

由用户下载HEX并加载本版配套AXF，不使用任何旧绝对地址。发生故障后保留power_status和power_event.fault。
原阶段/启动/恢复/参考/占空比字段仍可读取。新增重点：

| 字段 | 解释 |
| --- | --- |
| fault.run_age_samples | 接受启动后累计有效ADC周期数 |
| fault.foreground_age_samples / ui_entry_age_samples | 距前台/UI最近入口的周期数 |
| fault.ui_progress_age_samples / ui_page_age_samples | 距最近UI阶段/提交页面的周期数 |
| fault.foreground/ui.gap_samples_max | 已观测相邻入口最大ADC周期间隔 |
| fault.ui.frames_completed | 完整8页数据包已完成的帧数，不是提交次数 |
| fault.ui_frame_age_samples / cycles / ticks | 距上次完整帧完成的三种时钟差值 |

有效ADC周期数除150为ms、除150000为秒。32位计数约7.95小时回绕，覆盖本次1～2分钟观察。
它是现有Power_TimerIRQ成功消费ADC样本的计数，不是独立硬件墙钟：ADC丢失、IRQ未执行、调试暂停都会影响它。
不能把它当作ADC异常期间的真实经过时间。没有新增快速IRQ探针或额外计时器。
DWT差仍按32位取模，150MHz约28.63秒回绕；HAL tick也可能受调度影响。三种时钟分别保留，不能相减估算精确CPU占用。
相邻时间读取可能跨抢占，双缓冲只保证发布快照完整。calls/stage/page_submissions/frames_completed为0时，各自年龄0表示无数据。
UI阶段14表示FRAME_DONE，但随后可能立即发新页面，故障包更应看frames_completed和对应年龄。
完整帧完成只证明驱动收到数据包完成回调，不独立证明OLED供电/接线/物理像素正常。

先前约11.48秒的包不能直接代表用户后来描述的1～2分钟过程，尤其不能忽略DWT回绕。
故障后2.49V与万用表刷新速度不足以重建触发前的PA2采样点；电流保护解释应结合首故障原始ADC和波形。

## 验证

页面7饥饿、启动暂停丢失未发数据两个回归均先失败后通过。
真实UI+DMA联合测试覆盖频繁STARTING/RUNNING、长期静默、全部8页、整帧完成及故障/停止画面替换。
新增120秒模拟验证：DWT已回绕、HAL tick只推进11.484秒时，1800万个ADC周期年龄仍完整。
最终69组回归通过，另验证关闭事件诊断时的真实后台兼容性。独立只读源码审查未发现严重或重要问题；
一项次要验证建议留待补充：专门覆盖采样计数回绕及非零帧/采样字段发布中断与冻结的联合用例，当前仅有通用发布抢占/冻结测试。
120秒年龄测试不等于约7.95小时的专门回绕验证。

主/台架ARMCLANG6.18均重建0错误0警告。主Code49876/RO1312/RW44/ZI6476，主栈2048字节；台架栈1024字节。
CCM9692字节，执行0x10000000、加载0x0800A218。实际HEX拷贝/HRTIM/PendSV、CCM/普通SRAM/DMA与AXF导出逐字节一致性通过。
最终正常快速函数无FP指令，快速IRQ与worker无直接事件探针，台架HEX逐字节指纹保持。

- 主HEX SHA256：`E0A1963EEFFAC4E2E9399342CE7F9FFCC6BB4CD7B64FDA79105472D4B6A365BE`。
- 主AXF SHA256：`B406A4885115F5A6A3541F81AB4034FDC89BB1119B73EED516C9ED773710689A`。
- 台架HEX SHA256：`A9BA257BA3FFBB49C8FAED9803E3462671F692A9B4D063C9975512D09EF6D451`。

版本标签main-oledframe1-20261104，机器记录见FRM1-验证.json，下载/符号仍在原MDK-ARM/bidreaction目录。
本轮不操作或下载板子，由用户复测。源码/主机/镜像验证不能证明板上IRQ延迟、足够的RUNNING前台窗口、
电气稳定性、硬件OCP就绪或报告的分钟级冻结已消除；尚无本版实板运行与完整帧结果。
