# Forward Boost T7 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 减少MAIN正向Boost后台无效运算，准备保持控制行为的T7优化候选。
**Architecture:** 用既有模板生成mode2受保护专用入口，仅供真实worker使用；公共入口保持兼容。
**Tech Stack:** C11、STM32G474、ARMCLANG6.18、GCC、PowerShell、Keil。
**Spec:** `docs/superpowers/specs/2026-11-07-forward-boost-t7-design.md`

## Global Constraints

- 使用现有活动主工程及分支，保留四模式、唯一bench和原IDE改动。
- 150/50/10kHz、PI、门限、T1～T7、硬件OCP真实未确认状态保持；不操作板子。
- 修改前备份标签已远端核验；完成后提交并通过HTTPS推送、保留SSH origin、核对远端SHA。
- 生成物放`.superpowers/boost-t7-opt/`并忽略；测试参考源从不可变基线标签生成。

## Review Focus

- 专用入口仅在已校验mode2调用；错误模式锁存并清输出，以入口拒绝测试覆盖。
- 预充、接管、软启动及PI全部有效状态逐位等价，以两速率独立基线长轨迹测试覆盖。
- worker取消/故障/发布及T7期限保持，以既有真实后台抢占/截止期回归覆盖。
- 新函数CCM、实际镜像复制及worker分支正确，以链接映像和反汇编核验覆盖。
- 其他模式/bench保持，以完整回归、模式重初始化与bench HEX对照覆盖。

### Task 1: 受保护Boost入口、独立对比和实际映像验证

**Files:**
- Modify: `Core/Src/forward.c`, `Core/Src/forward_step_impl.inc`, `Core/Src/power_control.c`, `Core/Src/power_multirate.inc`, `MDK-ARM/bidreaction_ccm.sct`
- Create: `Core/Inc/forward_main_boost.h`, `tests/forward_boost_equivalence_test.c`, `tests/verify_forward_boost_image.ps1`
- Modify: `tests/run_tests.ps1`, `tests/multirate_switching_test.c`, `.gitignore`, `README.md`
- Create: `docs/正向Boost运行精简测试-20261108.md`

**Interfaces:** 输入为已有forward_U及初始化状态；新函数`void forward_main_boost_step(void)`；输出仍为forward_Y/B/X，不增加共享worker字段。

- [x] 编写ARM入口除法预算核验并在基线完整入口运行，预期RED：2条VDIV超过1条预算。
- [x] 编写独立旧源对比，覆盖两速率的全部B/X/Y、预充切换、随机/饱和/重初始化及入口拒绝。
- [x] 实现模板特化、后台调用和CCM放置；不改变原运算系数、状态更新或调度。
- [x] 运行对比及真实后台12→18V名义模型，预期逐位一致、启动/换向/停机保护通过。
- [x] 运行`tests/run_tests.ps1`全套、主/bench全量ARM构建、实际HEX/CCM/栈/DMA/AXF核验和除法预算，预期全部通过、bench HEX不变。
- [x] 独立只读审查，修正必要问题，记录软件验证边界；提交版本、推送分支和标签并核对远端SHA。
