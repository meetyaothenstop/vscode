# Forward Input ADC2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** MAIN正向Buck/Boost共用固定4.8us触发的ADC2输入采样。
**Architecture:** 集中正向输入源谓词，共用于请求快照和IRQ选择/有效性；沿用既有ADC2初始化、数据刷新和保护规则。
**Tech Stack:** C11、GCC、PowerShell、ARMCLANG6.18、Keil。
**Spec:** `docs/superpowers/specs/2026-11-08-forward-input-adc2-design.md`

## Global Constraints

- 固定活动主工程及当前codex/main-buck分支；保留四模式/唯一bench及原IDE改动，不操作板子。
- 150kHz采样/快速保护、Boost25/5与其余50/10计算、全部PI/门限保持；只统一正向输入数据源。
- 反向仍原ADC1；ADC2无效不回退；启动/重启/换向必须执行同一有效性策略。
- 备份标签已远端核验，完成后按长期授权提交推送并核对；不上传生成物。

## Review Focus

- 请求和运行对同一正向模式不能选不同ADC：raw/float真实后端两路差异测试。
- 缺EOC/EOS或OVR不能启动/继续：四种标志、全部阶段及换向测试。
- 停止/清故障后不能用故障前或反向样本启动：既有更新后的新鲜样本回归。
- 反向左侧是输出、不能错误替换为ADC2：独立反向残压及运行测试。
- 快速路径仍整数/CCM，bench保持：ARM反汇编和实际HEX核验。

### Task1: 统一输入来源并验证

**Files:** `Core/Src/power_control.c`, `Core/Inc/power_build_config.h`, `Core/Src/power_menu.c`, `tests/forward_input_phase_test.c`, `tests/power_control_test.c`, `tests/raw_backend_test.c`, `tests/run_tests.ps1`, `tests/main_menu_test.c`, `tests/main_ui_test.c`, `README.md`, `.gitignore`, `docs/正向输入统一ADC2测试-20261108.md`。
**Interfaces:** 保持既有Power_RequestStart/Power_TimerIRQ及PowerStatus结构，不新增正常路径诊断字段。

- [x] 编写真实后端双ADC差异、有效性、阶段及worker测试；在原代码见Boost选早相位的RED。
- [x] 统一请求/IRQ谓词与注释；更新旧测试中的Boost早相位预期和版本标识。
- [x] 定向/完整回归，主与bench构建0错误警告，bench HEX保持，CCM/向量/DMA/栈/AXF及快速整数映像核验。
- [x] 独立只读整体验证；README及测试记录，提交/标签/HTTPS推送并核对远端。
