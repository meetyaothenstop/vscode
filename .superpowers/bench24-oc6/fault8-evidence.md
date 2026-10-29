# 24V→12V旧1.5A门限首故障8

2026-10-29，用户确认旧`SW24-14`/目标12V正确，手动启动后报告故障8。
随后用户明确要求将最大电流提高到6A。更改/重置前已读取并保留该次证据。

方式：STM32CubeProgrammer2.16，SWD请求1000kHz/实际950kHz，HOTPLUG，core running。
未发送halt/reset/download/clear/start；本地完整日志`.superpowers/bench24/fault8-hotplug.log`
被忽略，已提取纯RAM字到本目录`fault8-raw-memory.txt`，不提交调试器唯一序列号。
[ST官方命令文档](https://dev.st.com/stm32cube-docs/prog/2.23.0/en/docs/markup/CubeProg_Command_Lines.html)
说明HOTPLUG连接不halt/reset；日志中的Reset mode描述配置，不是本轮执行复位的记录。

依据下载记录及用户OLED身份匹配旧候选F07116B0…，未进行整片Flash重新比对；
使用当时匹配AXF/map的地址，PowerStatus96字节位于0x20000A60。

| 锁存/保留项 | 值 |
| --- | --- |
| state/fault/detail | FAULT(4)/8/0 |
| 首故障输入 | 23.98608V（raw1368） |
| 首故障输出 | 6.0804605V（raw351） |
| 首故障电感采样电流 | 1.6527462A（raw3285） |
| 控制保留参考 | 7.170691V |
| 控制保留占空比 | 0.23285326 |
| 前一控制步U.IL | 0.49687386A |
| 前一控制步U.V_left/U.V_right | 24.003614V/5.3528843V |
| 电压误差B.Product2 | 1.0866013V |
| 电压积分B.DiscreteIntegrator | 0.35573313 |
| 电流参考B.Saturation | 0.6817135A（未达1A） |
| 电流误差B.Sum3 | 0.18483967A |
| 电流积分B.DiscreteIntegrator1 | -0.06865312 |
| isr_cycles_max | 773，已完成普通路径最大值，非此次故障IRQ耗时 |

故障8条件为`|IL|>=1.5A`或电流ADC端点；本次raw3285未在端点，按当前标定已超过1.5A。
首故障保护执行在本次控制输入更新之前，fault_*与U/B是相邻控制步，不能拼成同一步PI响应。
PI系数/抗饱和没有在此改动；已保留的参考约0.682A，并非碰到1A参考限幅才报错。
没有同次电流/输出电压/INA采样波形；这些数据不证明实际过流或误报的硬件根因。
新6A门限是用户指令的试验，故障样本可通过不等于问题已修复。
