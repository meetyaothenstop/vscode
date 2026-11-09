# 反向 Boost 检查与控制频率对齐

用户要求准备反向输入测试，将反向 Boost 检查对齐正向 Boost，并明确补充控制频率。

MAIN 四模式共用输入9～42V、启动|IL|<1A、输出残压≤3V、目标1～36V、
输出过压min(max(目标×1.5,6V),40V)。Boost仍要求目标严格高于实测输入；
Buck的0.95可达性检查保持。反向Boost使用右侧ADC1输入、左侧ADC1输出及负IL。

反向Boost后台改25kHz电流/5kHz电压，每6个PWM请求一次，T7截止40us。
积分步长40/200us、6238rad/s滤波复用boost_control_rates.h；反向连续PI及每秒斜坡保持。
PWM/ADC/快速保护150kHz，两个Buck50/10kHz。预充、全关断换向、HOLD/恢复与保护保持。

方案：共享既有门限选择器和Boost系数，模板生成mode4专用step/hold入口，
保留公共reverse_step/hold的50/10kHz兼容语义。直接改公共CONTROL_*会影响Buck；
复制整套控制源码会造成策略漂移，因此均不采用。此实现选择由助手提出。

沿用主工程与唯一bench，保留用户18→3V/12→6V默认及IDE修改；不操作板子。
基线70e5a84及pre-reverse-boost-align-20261109已上传核对。
完成标准：实际后端门限/节拍/快速故障/换向/HOLD测试、独立旧源同速率状态对比、
完整主机回归、主/bench ARM构建、实际镜像核验、审查、Git上传核对。
这些验证不等于实板时序、电气表现或硬件OCP就绪验证。
