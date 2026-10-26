# OLED刷新与T3超时的对照测试

实板已读到 fault=0x40、detail=3、fault_elapsed_cycles=0x76A=1898，
按150 MHz约12.65 μs，超过150 kHz的6.667 μs周期。故障时右侧采样
约0.03465 V。已确认本次deadline超限，尚未确认OLED是不是原因。

当前OLED绘图及轮询I2C在主循环，HRTIM IRQ优先级0，OLED/I2C代码
未屏蔽全局中断。OLED时间不直接计入ISR，但缓存、总线及中断副作用
仍需对照。此版本仅在启动准备/运行期间跳过绘图与I2C页面刷新，不改
DMA、控制环、ADC、PWM频率、时序门限和电流保护。

## 文件与标识

- 工程：`MDK-ARM/bench12_buck_oled_test.uvprojx`。
- 目标：`bench12_oled_mute`。
- HEX：`MDK-ARM/bench12_oled_mute/bidreaction_bench12_oled_mute.hex`。
- 首页：`12V SW OLED-MUTE`；参数标题含 `SW-MUTE`。
- 普通诊断版仍是 `bench12_buck.uvprojx` / `bench12_buck_sw`。

## 对照步骤

1. 两版都保持12 V/.2 A、100 Ω负载，同样的SWD设置。新工程预设SWD
   1 MHz，普通工程也需在Debug设置中核对为1 MHz，不能同时改变调试
   时钟和OLED条件。输出须先降到.25 V以下，输入/目标设置为12/6 V。
2. 烧录对照版并确认OLED-MUTE标识，再按start。按键扫描、start/stop
   及K4停机仍然执行；启动准备/运行期间暂停显示，故障或停机恢复刷新。
3. **屏幕可能仍停留在旧的STOPPED画面，即使PWM已经运行。屏幕不动
   不代表功率级停机。** 如果没有出现故障而画面一直不动，先按K4停机，
   确认显示恢复后，再在Keil暂停CPU；不要直接暂停仍在运行的功率级。
4. 出现故障，记录完整码和 `power_status.fault_elapsed_cycles`。
   若运行后由K4正常停机且未报错，读取 `power_status.isr_cycles_max`。
   新工程的Watch1预填了这些公共字段，便于暂停后读取。
5. 与同设置的普通诊断版比较。若关闭显示后仍是T3且耗时相近，OLED
   不是主要解释；若明显改善，则继续分离绘图、I2C和缓存影响，再决定
   DMA/执行路径优化。一次通过不代表闭环或硬件保护已经验证。

验证：16组主机测试通过，两版Keil0错误0警告。测试验证刷新门控、按键
停机及故障/停机显示恢复；实际CPU时序与OLED因果仍待实板对照。
