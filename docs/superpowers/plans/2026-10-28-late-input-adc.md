# FAST输入晚采样实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** 提供仅改变FAST输入电压采样相位的可构建对照固件。
**Architecture:** ADC1原三路注入采样保持；ADC2规则单通道在4.8µs采PA0，FAST使用其新鲜结果。
**Tech Stack:** STM32G474、ST HAL、HRTIM、C11主机测试、Keil ARM Compiler 6.18。
**Spec:** docs/superpowers/specs/2026-10-28-late-input-adc-design.md

## Global Constraints

- 唯一活动工程MDK-ARM/bench12_buck_fast.uvprojx，不复制工程。
- 保持6.6V输出保护、10–14V输入窗口、0.5A参考上限及1.5A采样过流。
- 不改PI参数、ADC1电流/输出采样时刻或现有840/936周期保护。
- ADC2故障不能退回ADC1早采样；主机测试不能冒充板上验证。
- 既有未提交IDE设置不覆盖、不纳入功能提交。

## Review Focus

- ADC2初始化失败时不得启动PWM计数器/使能输出。
- 缺少新鲜EOC/EOS或OVR必须拒绝启动并在活动阶段停机。
- 输入控制和保护不得仍取ADC1旧值，电流/右电压通道不得互换。
- 不能因晚采样修改旧工程或放宽任何保护门限。
- ADC2规则组必须维持每次硬件触发重新转换，禁止软件阻塞延时。

## Task 1: 输入采样配置与保护

**Files:** Core/Inc/power_build_config.h、Core/Src/power_control.c、tests/power_control_test.c、tests/power_hal/stm32g4xx_hal.h。
**Interfaces:** Power_Init接口保持；ADC2 DR成为FAST输入原始值；PowerStatus结构不变。

- [x] 扩展硬件边界fixture，先写独立ADC2初始化/触发、数据选择及坏样本保护测试。
- [x] 单独编译并运行FAST集成用例，确认原实现因缺少ADC2配置而失败。
- [x] 实现ADC2初始化校准、TA CMP4/TRG1配置以及EOC/EOS/OVR检查，保留ADC1路径。
- [x] 运行目标用例和完整tests/run_tests.ps1，核对旧工程行为不变。

## Task 2: 身份、ARM构建及记录

**Files:** Core/Src/power_menu.c、tests/bench12_menu_test.c、README.md、docs/Buck优化版测试.md、docs/板上测试记录.md。

- [x] 新标识先写可见行为测试再实现，区分当前ADC2对照固件。
- [x] 重建bench12_buck_fast，检查0错误0警告、CCM拷贝/向量、IRQ反汇编及HEX哈希。
- [x] 独立审查功能diff；记录主机/构建证据和待做板上验证，提交并保留单一活动工程。
- [ ] 等待采样时序实板对照结果；7V保护实验依赖该结果，本轮仍为6.6V。
