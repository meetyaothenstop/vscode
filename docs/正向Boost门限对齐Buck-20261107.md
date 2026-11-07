# MAIN正向Boost电气门限对齐Buck（2026-11-07）

## 用户确认与配置入口

在说明128 S9及正向Buck/Boost差异后，用户要求Boost限制与Buck一致，
并明确选择对齐全部电气和冷启动门限，反向Boost保留原值。

配置集中在`Core/Inc/power_build_config.h`的`POWER_MAIN_BUCK_*`组。
该组现同时供MAIN mode1/2/3使用，删除前一版单独的
`POWER_MAIN_FORWARD_BOOST_START_CURRENT_LIMIT_AMPS`副本，避免两个入口失配。

| 项目 | MAIN正向Buck/正向Boost/反向Buck | MAIN反向Boost（保持） |
| --- | --- | --- |
| 实测输入保护 | 10～42V | 6～40V |
| 冷启动电流 | 严格`|IL|<1A` | 严格`|IL|<0.25A` |
| 冷启动输出残压 | ≤3V | ≤0.25V |
| 运行输出过压线 | `min(max(目标×1.5,6V),40V)` | 目标×1.1 |
| 电流参考/绝对采样过流 | 6A/7.5A | 6A/7.5A |

例：正向Boost目标18V→过压线27V；26V→39V；27V或36V→40V。
3V只表示冷启动允许残压，不是启动参考，也不是运行过压线。
128 S6为请求时输出越限，S9为请求接受后的新采样或冷自举/装载阶段输出越限；
S8仍优先于S9，电流与输出同时不满足时保留S8。

## 实现与保持的行为

`MAIN_SHARED_ELECTRICAL_LIMITS`仅选择电气策略，`MODE_LIMIT`和过压上下限据此覆盖mode1/2/3。
接受请求时构建`active_limits`及`raw_policy`，后续整数快速IRQ和兼容浮点IRQ复用同一门限。
没有给正常150kHz路径增加新的模式判断、硬件读取或探针。

- 正向Boost启动仍要求目标严格高于实测输入，零目标拒绝，最大目标仍36V。
- Buck的0.95目标关系及零目标关断保持只用于Buck；不复制到Boost。
- 10～42V是软件采样窗口。输入41V、目标36V仍因升压关系报S7；输入42.1V先报S4。
- Boost仍使用ADC1原注入相位，不改用正向Buck的ADC2晚相位。
- Boost预充、全关换向、PI、软启动、T1～T7和原Boost写入时限保持。
- 反向Boost、唯一受限bench及普通历史配置保持；默认正向12→18V、反向12→6V保持。

## 验证范围

新增`tests/forward_boost_limits_test.c`，分别构建真实多速率整数后台和兼容浮点后台。
两条路径在旧代码下因拒绝2.99V残压出现预期RED，修改后均GREEN。

- ±0.98A与2.99V残压完成33周期启动；浮点路径检查3V等号允许、3.001V拒绝。
- 在待启动、自举、最后自举更新及两个装载边界注入3.05V，均报S9且无重新开通输出。
- 输入9.9V拒绝、10.1V/35V允许、41V因升压关系拒绝、42.1V因输入窗口拒绝。
- 等压测试使用ADC实际表示值，避免将量化后的约11.99V误写成精确12V；零目标和低于输入的目标拒绝。
- 用合成ADC事件检查运行输入上下界与缓存策略；这不证明Boost能在输出低于实际输入时稳压。
- 目标18/26/27/36V分别在27/39/40/40V的相邻ADC码验证过压边界。
- 反向Boost仍允许6.1V输入、拒绝41V输入和0.3V残压，并保留18V目标的19.8V过压线。
- 原启动1A测试、四模式保护断言和全4096码整数/浮点策略矩阵同步更新为新要求。

独立只读源码审查无Critical/Important/Minor问题；构建和完整回归由主助手执行，审查未独立重跑。

`tests/run_tests.ps1`完整75组回归通过，按脚本中的用例名称逐项核对75/75。
主/bench ARMCLANG6.18全量重建均0错误0警告；主Code50060/RO1312/RW44/ZI6508。
主HEX SHA256：`624DA1162750FC0567DD4D6959101F2DE2C6065D6F73BB3F6D1F127C77BDDDDD`。
主AXF SHA256：`03E753348236B53C0D54406B03B1D7D2A06073DCD60E8577F6C6D757FF10E22C`。
bench HEX SHA256：`A9BA257BA3FFBB49C8FAED9803E3462671F692A9B4D063C9975512D09EF6D451`，与修改前逐字节一致。

## Git与交付边界

修改前基线：`7c985e9fa1a4df9f680bfaa3b4b9f63091b281f2`。
备份标签`pre-forward-boost-align-20261107`在编辑固件前已通过HTTPS上传并核对远端解引用提交。
版本标签`main-forward-boost-align-20261107`，SSH origin保持，不强推。
主工程仍为`MDK-ARM/bidreaction.uvprojx`，唯一受限bench仍为`MDK-ARM/bench12_buck_fast.uvprojx`。
主HEX为`MDK-ARM/bidreaction/bidreaction.hex`，调试使用同次AXF；标识仍为ADG2，事件版本4。
既有IDE配置和无关未跟踪文件保持，不提交二进制或IDE状态。

本次未下载、复位、启动或操作板子。软件门限放宽不证明S8/S9/故障4的实际原因消除，
也不证明实板最坏时序、稳压或硬件OCP接线/就绪。用户继续自主下载和板测。
