# CCM执行耗时对照：2026-10-26

## 本轮实板证据

Flash统一诊断版报64 T3：stage=7、entry_state=POWER_ARMING、valid_mask=0x3F，
entry_spacing_cycles=0x3E8=1000，entry_counter_ticks=0x1720=5920。
结合当前正向Buck代码，是arming开放输出、转入运行并计算首个控制结果的那一轮。
这不是持续运行已被验证，也不能据输出未触发推断门极从未产生脉冲。

| 完成点 | 累计周期 | 与前一完成点差值 | 区域时间（150MHz） |
| --- | ---: | ---: | ---: |
| 入口检查 | 0xC0=192 | 192 | 1.280μs |
| ADC读取/换算 | 0x12E=302 | 110 | 0.733μs |
| 保护检查 | 0x20F=527 | 225 | 1.500μs |
| 状态准备 | 0x2DF=735 | 208 | 1.387μs |
| 控制器计算 | 0x6F9=1785 | 1050 | 7.000μs |
| PWM映射 | 0x7EA=2026 | 241 | 1.607μs |

累计2026周期约13.507μs；写入前预算仍为840周期/5.6μs。各段包括探针开销，
不是纯函数基准。完成点6未置有效位，0不表示真实寄存器写入耗时为0。

这些数据明确控制器区域是最大热点，但并未证明Flash访问就是根因。先进行
只改变执行位置的对照，保留控制算法，避免同时改PI、频率、门限或OLED DMA。

## 新版文件

- 工程：`MDK-ARM/bench12_ccm_diag.uvprojx`
- Target：`bench12_ccm_diag`
- HEX：`MDK-ARM/bench12_ccm_diag/bidreaction_bench12_ccm_diag.hex`
- 首页：**12V CCM DIAG**；参数标题含**SW-CCM**。
- 原Flash版保留：`bench12_unified_diag.uvprojx`，首页12V UNIFIED DIAG。
- Watch沿用`power_diag.fault`和`power_diag.fault.profile`，不要使用旧AXF符号。

仅将控制IRQ、入口包装、enable_outputs、forward_step、PWM映射及保护轮询
放入CCM执行。原指令所实现的控制行为保持；CPU150MHz、PWM150kHz、ADC
TRG2/CMP2、输出使能顺序、0.5A参考/1.5A采样过流、0.25A/0.25V近零检查和
840/936周期时限保持。OLED仍使用启动/运行期间暂停刷新的策略，未改DMA。

STM32G474xB的32KiB CCM位于0x10000000并别名映射至0x20018000。
自定义scatter把普通数据/栈限制到0x20000000～0x20017FFF，排除别名重叠。
代码的加载副本仍在Flash，标准__main scatter启动过程负责复制；没有手工
调用初始化顺序、VTOR重映射或直接把HEX烧进易失RAM。
依据：[ST DS12288第3.5节](https://www.st.com/resource/en/datasheet/stm32g474cb.pdf)。

## 构建验证与限制

ARM构建已得到3684字节CCM区，执行地址0x10000000、加载地址0x08007B28。
验证脚本从实际MAP和HEX检查热函数地址、IRQ向量、复位/紧急停机Flash位置、
RW边界和scatter复制表项；原Flash版被此检查正确拒绝。
这证明构建配置和下载镜像包含所需复制数据，不能代替实板冷启动和耗时复测。

## 下一次只做一次同条件对照

1. 保存现有Flash版截图。用户手动下载CCM版后，确认首页标识及Target；保持
   12V、限流0.2A、100Ω、目标6V及原来的SWD1MHz。确认右输出已放电并满足原
   启动准备条件。先确认冷启动后界面/采样正常，CPU正常运行，再按start一次。
2. 若故障，确认已停机后再暂停Keil，记录本次故障码、`power_diag.fault`及
   `profile.checkpoint_cycles`。同T3时比较各段差值；若变成S8/S9，按新版状态
   和同次采样单独分析，不把它当作T3已全面解决的证明。
3. 若画面冻结且没有明确故障，先按K4停机，显示恢复后才暂停CPU；记录
   `power_status.isr_cycles_max`及右侧万用表电压。冻结画面不等于功率已停止。

最终是否达到时限、实际冷启动是否正常，以及门极/死区、采样准确性和闭环，
仍需实板验证。助手仅准备/构建文件，没有下载或自动启动硬件。
