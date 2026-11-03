# 反向Buck HOLD分步恢复候选RES1 — 2026-11-03

主工程仍为`MDK-ARM/bidreaction.uvprojx`、Target`bidreaction`，生成
`MDK-ARM/bidreaction/bidreaction.hex`。首页`MAIN ADC RBUCK RES1`，参数页`M-RRES1`。
用户自行下载；本轮没有下载、启动调试器或操作板子。

## 本次现场及原因

W128后用户反馈12V右输入、6V左输出报64 T5。截图a9f86bc7记录：

| 字段 | 值 |
| --- | --- |
| fault/detail | 0x40 / 5 |
| elapsed/spacing/counter | 0x38B=907 / 0x3E8=1000 / 0x7960=31072 |
| isr_cycles_max | 907 |
| reference / fault_v_left | 4.38342094V / 4.38342094V |
| fault_v_right / fault_il | 11.9703655V / −0.0931873322A |
| d_cmd / retained left,right compare | 0.364791572 / 28160,11522 |

用户确认目标6V；随后截图60c4ac01用W128当前MAP地址读出bootstrap_resume=1、
bootstrap_cycles=0x1E=30。这确认执行到HOLD恢复后的begin_bootstrap(1)分支。
恢复重置参考/控制器、计算/映射，加上末尾低侧开通及前后硬件故障检查，集中在同一IRQ。
907未超过相对退出线936，但31072超过绝对退出线29952，共1120ticks=35CPU周期；
距32000仅29CPU周期。本次不再放宽时序线。为何进入HOLD及输出采样降到4.383V仍未确定，
Flash身份及电气波形也未独立验证，不能把调度原因当作输出变化的物理根因。

## 本次修改

仅MAIN mode3恢复流程增加一个全部关断的准备间隔，约6.667us：

1. HOLD满足原恢复条件时，原控制器计算并缓存比较值；保持所有门极关闭，
   标记私有pending、state=POWER_BOOTSTRAP。此时bootstrap_cycles=31表示尚未开低侧。
2. 下个IRQ仍先做新鲜ADC、输入/输出/电流等原保护，再复查输出<目标×0.99且|IL|<0.25A。
   通过后才开低侧、设置30周期；未通过则返回HOLD、保持全关断。
3. 完整30个低侧周期后写入缓存比较值，再等2个装载周期才开全部输出。
   新增的早期开通和取消分支都经过原T5/T6检查，不叠加控制器计算。

STOP、0V、清故障、首次故障和新启动请求取消pending。公开状态/命令ABI不变，
已有BOOTSTRAP启动显示会静默OLED。保存的plan比旧流程多等待一个样本周期；开低侧前有新鲜资格复查。
PI、ADC/PWM/死区、全部电压/电流保护及其他模式/台架策略保持。
写前840/27904、退出936/29952、入口T1/T2和pending REP/T6均保持。

## 验证

- 旧代码上的新增真实后台测试先失败：准备周期已启用LOW_OUTPUTS；修改后通过。
- 48组完整回归通过：含全关断准备、新样本电压及正/负电流取消、完整30+2周期、
  缓存比较值跨共同边界、STOP/零目标/清故障、ADC/输入/输出/电流故障及硬件短故障双检查。
  准备、早期开通、取消三路径均测试T5/T6、936/937、29952/29953及优先级；
  原907/31072仍必须报T5，首故障证据保留。原控制器位一致性及四模式回归继续通过。
- MAIN反向理想开关模型加入一个准备停顿；12→6V的100Ω、12Ω及近空载终值约5.999/5.999/6.000V。
  模型不重现本次板上输出波动，也不证明开关时序或电气波形。
- 独立只读代码审查未发现保留的Critical/Important/Minor。
- 主/台架ARM6.18重建均0错误0警告。主Code47048/RO1320/RW44/ZI4508，
  CCM9060字节、run0x10000000/load0x0800998C。实际HEX/CCM启动拷贝/向量/无探针/DMA核验通过。
  ARM确认mode3晚分支仅缓存/置pending，不调用enable_outputs；下一IRQ早分支调用使能并转共同退出检查。
  普通硬件成对写入仍13条指令（LDRD至最后CR1写，不含两个状态写）；与W128操作顺序一致。
  编译器寄存器及编码改变，不以指令条数推断真实最坏耗时。首页立即数及参数HEX身份核对通过，AXF导出HEX一致。
- HEX SHA256：`B0231D707403A763FA916BFD5CF72535160FB7ABA88B3E756B295D031CF97A55`。
  AXF SHA256：`8223BEB4294607A39399230202A3D3A534849EA72600CFE84A4CF216F79093B3`。
  台架HEX保持`A9BA257BA3FFBB49C8FAED9803E3462671F692A9B4D063C9975512D09EF6D451`。
  当前日志位于`.superpowers/reverse-resume/`，二进制/日志不提交Git。

## 用户下一步

下载本HEX并核对RES1首页，继续右12V输入、左6V目标。若再故障，保持同一次现场，
记录fault/detail、elapsed/spacing/counter、isr_cycles_max、fault_v_left/right/il及v_reference。
调试器应加载配套新AXF，重新添加符号；旧W128绝对地址不再适用。

本版MAP：power_status=0x20000B14，pending=0x200001AC(1字节)、
bootstrap_resume=0x200001B4(1字节)、bootstrap_cycles=0x20000200(4字节)。
仅本指纹适用；若静态名无法求值，可按对应宽度只读地址。不得沿用上一版0x200001B0/0x200001FC。

本版待用户板测：T5是否消除、HOLD原因、实际稳压/峰值、完整异常返回最坏余量及硬件OCP接线均未新增验证。
