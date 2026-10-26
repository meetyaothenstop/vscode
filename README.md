# STM32G474 双向 Buck-Boost 台架调试

后续固定维护 **`MDK-ARM/bench12_buck_fast.uvprojx`**，Target 为
`bench12_buck_fast`。通过 Git 提交和标签保存历史；普通调试修改不再复制工程。

## 当前版本

- OLED 首页 `12V BUCK FAST`，参数页 `SW-FAST`。
- 受限正向 Buck：12V 输入台架，6V 默认目标，100Ω 负载。
- 准备阶段 `|采样电流| < 0.5A`、右输出电压 `<= 1V`。
- 当前测试工程输入允许10.5–14.0V；按用户要求把原13.5V上限增加0.5V余量，
  启动及运行统一检查，超过14V仍按单次采样立即关断。
- 电流参考上限0.5A，采样过流1.5A；硬件过流保护接线尚未验证。
- 控制热路径放置于 CCM；裁掉未参与 Buck 的 Boost 计算。
- 840周期写入、定时器相位和936周期收尾等时序保护仍启用。
- OLED 活动期间暂停刷新，尚未使用 DMA。

最近软件验证：25组主机测试通过，54万个采样点对比通过；Keil 编译0错误0警告。
最新板上反馈（2026-10-26）：Buck FAST 连续计算至参考约0.601V后触发输入保护2，
用户报告 IN13.53V；此前记录中断最大耗时755周期，首次控制计算的T3障碍本次已跨过。
用户随后用万用表及示波器观察输入约11.9–12V，因此采样干扰需继续核查。
完整6V运行与更长时间的时序仍待验证。详见[板上测试记录](docs/板上测试记录.md)，
操作步骤见[Buck优化版测试](docs/Buck优化版测试.md)。

## 以前使用的工程

| 工程文件（MDK-ARM 下） | 用途 |
| --- | --- |
| `bidreaction.uvprojx` | 通用工程 |
| `ocp_verified_build.uvprojx` | 硬件保护配置验证 |
| `bench12_buck.uvprojx` | 12V 受限 Buck 台架 |
| `bench12_buck_oled_test.uvprojx` | OLED 活动期间暂停刷新对照 |
| `bench12_unified_diag.uvprojx` | 分段耗时与首故障诊断 |
| `bench12_ccm_diag.uvprojx` | CCM 放置及耗时诊断 |
| `bench12_ccm_base.uvprojx` | CCM 无额外耗时插桩对照 |
| `bench12_buck_fast.uvprojx` | 当前维护的 Buck 优化工程 |

这些工程共享 `Core` 和 `Drivers`。这次 Git 初始提交记录的是当前文件状态，
不是过去各次测试时的完整源码历史；旧工程重新编译会使用当前共享源码。
此前的源码备份、诊断记录、构建脚本和固件哈希保存在 `.superpowers` 相关目录，
设计与测试文档在 `docs`。原先嵌套的 `bidreaction` 目录作为已有参考资料保留。

## 构建和版本记录

Keil MDK 工程使用本机 ARM Compiler 6.18；生成的 HEX 位于
`MDK-ARM/bench12_buck_fast/bidreaction_bench12_buck_fast.hex`。
编译产物可以重新生成，由 `.gitignore` 排除。

运行主机回归：

```powershell
./tests/run_tests.ps1
```

普通修改在同一个工程进行，完成适当验证后提交。记录板上测试结果时，注明
提交号、输入电压、负载、故障码和关键观测值；确认有意义的测试节点后再打标签。
首次导入不能标记为“硬件已通过”。通过 Git 查看/恢复旧文件时，不需要复制工程目录。
