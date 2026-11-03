# EVT1故障阶段与前台进度诊断

2026-11-03。用户批准在RAW1基础上添加定位信息，保持PWM/ADC/软件保护150kHz、电流50kHz、电压10kHz、PI及保护门限。
改前9652d4b已用pre-eventdiag-20261103上传并核对远端SHA。仍使用主工程bidreaction.uvprojx，用户下载。
首页MAIN ADC EVT1 50/10，参数页M-EVT1。固件标识改变，不表示故障16或OLED停更已经修复。

## 实现边界

power_event是独立全局Watch对象，不再依赖文件内静态变量或绝对地址。
正常快速IRQ/worker不增加诊断探针；故障记录在输出关断后、取消控制前，在原首故障锁内一次捕获。
Power_Service入口和UI阶段由前台写入双槽邮箱，DMB后用单个32位索引发布，不新增关中断区。
故障若打断未发布的写入，读取旧的完整槽。记录valid最后写1，之后前台继续运行/其他故障/Stop不能覆盖；
成功清故障才清除首故障，失败的确认保留它。每次接受新启动重置进度邮箱并增加run_id。

## 用户复测

由用户下载主HEX并加载本版AXF，确认EVT1，仍测试右12V→左6V、100Ω。
故障后保留现场，在Watch添加power_event，展开fault；同时保留power_status首故障电压/电流/原始ADC。
不要沿用RAW1静态地址。事件fault与原power_status故障字段由同一个首故障锁保护。

| power_event.fault字段 | 含义 |
| --- | --- |
| valid/code/detail/state | 首故障是否完整、故障码/细节、被POWER_FAULT覆盖前的状态 |
| mode/target_volts/start_pending | 当前后台配置/待启动标志；启动准备过程中mode/target可能是暂定值，不能当作已接受请求的原子配置对 |
| bootstrap_resume/bootstrap_remaining/arming_remaining/resume_pending | 恢复历史与故障当次剩余阶段，不再用故障后的变量猜测 |
| polarity_samples | 反向错误方向连续样本计数 |
| duty/reference_volts/left_compare/right_compare | 最后已提交的软件指令及预装载比较值，不是示波器实测门极或PWM活动影子寄存器 |
| foreground/ui.calls | 本次接受启动之后的前台/UI入口次数；UI阶段标记不增加calls |
| foreground_age_cycles/ticks | 故障捕获时距最近Power_Service入口多久 |
| ui_entry_age_cycles/ticks | 距最近PowerUI_Service入口多久 |
| ui_progress_age_cycles/ticks | 距最近UI阶段发布多久 |
| ui.page_submissions/last_page、ui_page_age_cycles/ticks | 本次运行接受过几次页面提交、上次页号及其年龄；持续BUSY的扫描不会刷新这组时间 |
| ui.stage/page/io_status | 最近完整发布的UI阶段、所处理的页/next_page、HAL状态 |
| foreground/ui.gap_cycles_max | 本次运行已观测的相邻入口最大间隔，不包含尚未返回的末次长等待；配合age看 |
| capture/clock_ready/run_age/run_id | 捕获时钟是否已启用及运行代号；clock_ready也随首故障冻结；没有接受启动时run_age为0 |

state：STOPPED0、BOOTSTRAP1、ARMING2、RUNNING3、FAULT4、CHANGEOVER5、HOLD6。
ui.stage：ENTRY1、KEYS2、POLL3、WAIT_DMA4、STARTUP_MUTED5、WAIT_REFRESH6、RENDER7、
FRAME_READY8、SUBMIT_PAGE9、PAGE_QUEUED10、PAGE_BUSY11、IO_ERROR12、IDLE13。
HAL状态OK0/ERROR1/BUSY2/TIMEOUT3。PAGE_QUEUED只表示DMA提交被接受，不代表整帧实际显示完毕。
page是0..7，8表示没有待发送页/下一帧尚未构建。

若foreground与UI入口age都很大，支持前台长时间没推进的假设；若入口正常、ui_page_age很大且UI为WAIT_DMA/IO_ERROR，
重点查传输。没有任何页面提交时page_submissions=0、ui_page_age=0占位，结合run_age和入口次数判断始终未提交；
不能把占位0理解成刚发送。如果停在RENDER/SUBMIT_PAGE，说明最近发布的执行位置。阶段可能在故障发生前刚切换，
单个包不能替代持续运行轨迹，不能仅凭一个阶段直接断定卡死。

周期数除150得到us。DWT32位在150MHz约28.63秒回绕；差值按模32位，单次间隔超过一个回绕无法恢复。
HAL tick是另一时钟，不能先验假定在CPU饥饿时仍准时推进。前台cycle/tick为相邻读取，可能跨抢占，
不能由二者差值精确估算CPU占用。calls=0表示本次启动后尚无该入口，入口age的0是无数据占位。
在同一次UI服务中接受启动时入口记录被清零，但后半次服务仍会发布阶段；此时ui.calls=0而ui.stage非0，
ui_progress_age仍有效，ui_entry_age仍为占位0。stage=0才表示没有阶段记录。
只在BOOTSTRAP/ARMING状态记录对应剩余计数，其他状态为0，避免旧运行残留。
启动前采集的故障可能fault.clock_ready=0，不能解释成正常DWT计时。首故障捕获在关断之后，age不是门极关断的精确延迟。

## 验证

新增真实后台测试覆盖阶段/首故障冻结与清除/无心跳/发布中被IRQ打断/计数回绕，UI夹具覆盖启动静默、DMA等待、渲染和提交失败。
独立只读审查指出3项重要诊断缺陷：启动当次后半段UI阶段的年龄被误置0、重复BUSY扫描掩盖页面提交停滞、
上次失败启动的装载计数可能残留到新故障。三个用例均先失败再修正通过；另补时钟就绪在首故障中冻结的RED→GREEN用例。
审查后由作者回归/镜像复核，没有声称二次独立审查。启动准备期间mode/target暂定值为已说明的次要限制，
本版未额外保存“已接受配置对”；RUNNING/HOLD故障使用当前已接受配置。

最终66组回归全部通过；关闭POWER_EVENT_DIAGNOSTICS的真实后台兼容测试也通过。
主/台架ARMCLANG6.18重建均0错误0警告。主Code49472/RO1320/RW44/ZI6244；
CCM9692字节，执行0x10000000、加载0x0800A08C；主栈2048字节，台架1024字节。
实际HEX启动拷贝/HRTIM/PendSV、CCM/DMA/SRAM、AXF导出逐字节一致性通过。
新增检查确认event_capture/event_publish/PowerEvent_UIMark在Flash、power_event在普通SRAM，
快速IRQ及worker无直接事件探针，原整数快速函数无FP指令。旧RAW1镜像不含事件函数，新增镜像验证先失败再对EVT1通过。
本版不启用原逐IRQ统一诊断power_diag；前台标记仍消耗CPU并改变链接布局，这些检查不能保证实板WCET或“零观测开销”。

- 主HEX SHA256：`C8BA5AD2686512F89992D31F3282BB414555903AF222EF9A37371D1B0515936B`。
- 主AXF SHA256：`C1A63F4F7111BFB2200B63341D6B6B912CCA74A870CA0ACFECDC1F310255707B`。
- 台架HEX SHA256：`A9BA257BA3FFBB49C8FAED9803E3462671F692A9B4D063C9975512D09EF6D451`，与改前一致。

版本标签main-eventdiag1-20261103，机器记录见EVT1-验证.json。主工程HEX/AXF仍在原MDK-ARM/bidreaction目录，不提交可重建二进制。
本轮未操作/下载板子；主机结果不证明板上最坏时序、电气稳定性或硬件OCP接线。
