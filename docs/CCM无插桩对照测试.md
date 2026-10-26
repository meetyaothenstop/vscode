# CCM无分段插桩对照：2026-10-26

## 为什么做这一版

最新CCM诊断截图有效位0x3F，累计点为101、184、289、396、890、1012周期，
区域差值101、83、105、107、494、122。entry_counter=0xE60。此前Flash截图
控制器1050、累计2026；本次控制器494、累计1012，但截图没有显示entry_state，
不能把两次数据当作严格同阶段基准。当前仍超过840周期写入前时限。

先关闭可选分段探针，使用原生时序保护测CCM下的真实运行预算，再判断是否
需要改控制器实现。**没有删除DWT超时监测、放宽门限或改变控制算法。**

## 本轮使用的文件

- 工程：`MDK-ARM/bench12_ccm_base.uvprojx`
- Target：`bench12_ccm_base`
- HEX：`MDK-ARM/bench12_ccm_base/bidreaction_bench12_ccm_base.hex`
- 首页：**12V CCM BASE**；参数标题含**SW-BASE**。
- 关键编译宏：bench=1、OLED-mute=1、CCM=1、unified-diagnostics=0、OCP确认=0。
- 分享原来已验证的`bench12_ccm_diag.sct`；普通RAM仍限制96KiB，避开CCM别名。

原生PowerStatus仍锁存故障码/子码、三项ADC原始码及物理量、T故障精确耗时、
入口间隔和计数器。启动电流/残压、1.5A采样过流、输入/输出保护、ADC、
控制器/PWM校验、840/936周期门限和输出关断顺序都保留。参考电流仍0.5A。
CPU150MHz、PWM150kHz、采样触发和PI参数不变。OLED仍为运行期间静默，未改DMA。

`power_diag`和私有分段探针被编译移除；本版不要用旧的power_diag观察项。
Watch1已预填本轮字段。独立目标使用其自己的AXF，避免用旧符号解释新RAM地址。

## 下一次只做一次复测

保存当前截图后，用户手动下载本版，核对Target和首页。保持12V、限流0.2A、
100Ω、目标6V和SWD1MHz。确认输出放电和原启动条件后，CPU正常运行，按start一次。

- 若故障，确认已停机后再暂停Keil。保存Watch1完整截图，重点是
  `power_status.fault`、`fault_detail`、**`fault_elapsed_cycles`**及`v_reference`。
  T3时fault_elapsed_cycles是写前实际测量，isr_cycles_max可能仍为0。
- 若是S8/S9，先保存fault_il、fault_v_right及原始码。这次可能在控制计算之前
  就拒绝启动，不能以没有T3判定时序问题已解决。
- 若没有明确故障但OLED画面冻结，先按K4停机；显示恢复后再暂停，读取
  **`power_status.isr_cycles_max`**。画面冻结不能作为停机标志。

在本轮受限正向Buck冷启动中，参考从0.1V以120V/s上升。T3检查之前已经更新
v_reference，因此它接近0.1V有助于识别首个控制步；较高值表示已推进后续步骤。
这一推断只用于本配置、同一次T3现场，不能解释T1/T2/S8/S9的旧实时参考值。

本版不提供分段阶段记录；换取移除诊断开销。若原生时序仍超限，应依据本次
fault_elapsed_cycles决定进一步优化，而不是取消时序保护。

## 已验证及未验证

21组主机测试通过，新增无插桩配置跑同一后端保护回归和菜单身份测试；
ARM构建0错误0警告。实际MAP/HEX验证4228字节热代码位于CCM，Flash加载地址
0x08007A70，标准复制表及IRQ向量有效，诊断探针对象不存在；原插桩CCM镜像
被“无插桩”检查正确拒绝。静态代码大小变化不能用来推断实际提速。

真实时限余量、冷启动、输出6V稳定性、采样准确性、门极/死区及保护时延仍需
实板确认。助手只构建文件，没有自动下载或启动硬件。
