$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    $referenceObject = Join-Path $PSScriptRoot 'forward_reference.o'
    $renameFlags = @('configure','initialize','step','terminate','U','Y','B','X','errorStatus','sampleTime','checksum') | ForEach-Object { '-Dforward_' + $_ + '=reference_forward_' + $_ }
    & gcc -std=c11 -O2 -Wall -Wextra -Werror -ICore/Inc @renameFlags -c Core/Src/forward.c -o $referenceObject
    if ($LASTEXITCODE -ne 0) { throw 'Reference controller compile failed' }
    $fastCoreFlags = @('-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-DPOWER_CCM_CONTROL=1','-DPOWER_BENCH_FAST_BUCK=1')
    $fastFlags = $fastCoreFlags + @('-DPOWER_BENCH_24V_BUCK_TEST=0')
    $bench24Flags = $fastCoreFlags + @('-DPOWER_BENCH_24V_BUCK_TEST=1')
    $mainFlags = @('-DPOWER_MAIN_APPLICATION=1','-DPOWER_MAIN_ADC_SOFTWARE_ONLY=1')
    $mainDiagFlags = $mainFlags + @('-DPOWER_UNIFIED_DIAGNOSTICS=1')
    $cases = @(
        @{Name='main_buck_plan_test'; Sources=@('tests/pwm_buck_plan_test.c','Core/Src/pwm_plan.c'); Flags=$mainFlags}
        @{Name='main_diag_power_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=($mainDiagFlags + @('-Itests/power_hal','-Wno-unused-function'))}
        @{Name='main_diag_menu_test'; Sources=@('tests/main_menu_test.c','Core/Src/power_menu.c'); Flags=$mainDiagFlags}
        @{Name='main_forward_test'; Sources=@('tests/main_forward_test.c','Core/Src/forward.c',$referenceObject); Flags=$mainFlags}
        @{Name='main_power_control_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=($mainFlags + @('-Itests/power_hal','-Wno-unused-function'))}
        @{Name='main_phase_budget_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=($mainFlags + @('-Itests/power_hal','-Wno-unused-function','-DPOWER_TEST_TIMING=1'))}
        @{Name='main_power_guard_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=@('-DPOWER_MAIN_APPLICATION=1','-DPOWER_MAIN_ADC_SOFTWARE_ONLY=0','-Itests/power_hal','-Wno-unused-function')}
        @{Name='main_menu_test'; Sources=@('tests/main_menu_test.c','Core/Src/power_menu.c'); Flags=$mainFlags}
        @{Name='main_ui_test'; Sources=@('tests/main_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c'); Flags=$mainFlags}
        @{Name='main_menu_guard_test'; Sources=@('tests/main_menu_test.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_MAIN_APPLICATION=1','-DPOWER_MAIN_ADC_SOFTWARE_ONLY=0')}
        @{Name='main_ui_guard_test'; Sources=@('tests/main_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_MAIN_APPLICATION=1','-DPOWER_MAIN_ADC_SOFTWARE_ONLY=0')}
        @{Name='main_oled_dma_test'; Sources=@('tests/oled_dma_test.c','Core/Src/OLED_IIC_Config.c'); Flags=$mainFlags}
        @{Name='main_buck_switching_test'; Sources=@('tests/bench12_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c'); Flags=$mainFlags}
        @{Name='main_boost_switching_test'; Sources=@('tests/pwm_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c'); Flags=$mainFlags}
        @{Name='main_reverse_switching_test'; Sources=@('tests/reverse_switching_test.c','Core/Src/reverse.c','Core/Src/pwm_plan.c'); Flags=$mainFlags}
        @{Name='power_bench24_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=($bench24Flags + @('-Itests/power_hal','-Wno-unused-function'))}
        @{Name='bench24_switching_test'; Sources=@('tests/bench12_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c'); Flags=$bench24Flags}
        @{Name='bench24_menu_test'; Sources=@('tests/bench12_menu_test.c','Core/Src/power_menu.c'); Flags=$bench24Flags}
        @{Name='bench24_ui_test'; Sources=@('tests/power_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c'); Flags=($bench24Flags + @('-Wno-unused-function'))}
        @{Name='forward_buck_equivalence_test'; Sources=@('tests/forward_buck_equivalence_test.c','Core/Src/forward.c',$referenceObject); Flags=$fastFlags}
        @{Name='power_buck_fast_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=($fastFlags + @('-Itests/power_hal','-Wno-unused-function'))}
        @{Name='buck_fast_switching_test'; Sources=@('tests/bench12_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c'); Flags=$fastFlags}
        @{Name='buck_fast_menu_test'; Sources=@('tests/bench12_menu_test.c','Core/Src/power_menu.c'); Flags=$fastFlags}
        @{Name='adc_measure_test'; Sources=@('tests/adc_measure_test.c','Core/Src/adc_measure.c')},
        @{Name='adc_acquire_test'; Sources=@('tests/adc_acquire_test.c','Core/Src/adc_acquire.c','Core/Src/adc_measure.c')},
        @{Name='pwm_control_test'; Sources=@('tests/pwm_control_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c')}
        @{Name='pwm_switching_test'; Sources=@('tests/pwm_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c')}
        @{Name='reverse_control_test'; Sources=@('tests/reverse_control_test.c','Core/Src/reverse.c','Core/Src/pwm_plan.c')}
        @{Name='reverse_switching_test'; Sources=@('tests/reverse_switching_test.c','Core/Src/reverse.c','Core/Src/pwm_plan.c')}
        @{Name='power_control_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=@('-Itests/power_hal')}
        @{Name='power_bench12_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=@('-Itests/power_hal','-DPOWER_BENCH_12V_BUCK_TEST=1','-Wno-unused-function')}
        @{Name='power_unified_diag_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=@('-Itests/power_hal','-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-DPOWER_UNIFIED_DIAGNOSTICS=1','-Wno-unused-function')}
        @{Name='power_ccm_base_test'; Sources=@('tests/power_control_test.c','Core/Src/power_control.c','Core/Src/forward.c','Core/Src/reverse.c','Core/Src/pwm_plan.c','Core/Src/adc_measure.c'); Flags=@('-Itests/power_hal','-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-DPOWER_UNIFIED_DIAGNOSTICS=0','-DPOWER_CCM_CONTROL=1','-Wno-unused-function')}
        @{Name='bench12_menu_test'; Sources=@('tests/bench12_menu_test.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_BENCH_12V_BUCK_TEST=1')}
        @{Name='unified_diag_menu_test'; Sources=@('tests/bench12_menu_test.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-DPOWER_UNIFIED_DIAGNOSTICS=1')}
        @{Name='ccm_diag_menu_test'; Sources=@('tests/bench12_menu_test.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-DPOWER_UNIFIED_DIAGNOSTICS=1','-DPOWER_CCM_CONTROL=1')}
        @{Name='ccm_base_menu_test'; Sources=@('tests/bench12_menu_test.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-DPOWER_UNIFIED_DIAGNOSTICS=0','-DPOWER_CCM_CONTROL=1')}
        @{Name='bench12_switching_test'; Sources=@('tests/bench12_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c'); Flags=@('-DPOWER_BENCH_12V_BUCK_TEST=1')}
        @{Name='power_protection_test'; Sources=@('tests/power_protection_test.c','Core/Src/power_protection.c'); Flags=@('-Itests/power_hal','-DPOWER_OCP_WIRING_CONFIRMED=1')}
        @{Name='power_protection_unconfirmed_test'; Sources=@('tests/power_protection_test.c','Core/Src/power_protection.c'); Flags=@('-Itests/power_hal','-DPOWER_OCP_WIRING_CONFIRMED=0')}
        @{Name='power_menu_test'; Sources=@('tests/power_menu_test.c','Core/Src/power_menu.c')}
        @{Name='power_ui_test'; Sources=@('tests/power_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c')}
        @{Name='power_ui_oled_mute_test'; Sources=@('tests/power_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c'); Flags=@('-DPOWER_BENCH_12V_BUCK_TEST=1','-DPOWER_TEST_SKIP_OLED_ACTIVE=1','-Wno-unused-function')}
        @{Name='power_ui_live_test'; Sources=@('tests/power_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c'); Flags=($fastFlags + @('-Wno-unused-function'))}
        @{Name='power_ui_fast_mute_test'; Sources=@('tests/power_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c'); Flags=($fastFlags + @('-DPOWER_BENCH_LIVE_OLED=0','-Wno-unused-function'))}
        @{Name='oled_dma_test'; Sources=@('tests/oled_dma_test.c','Core/Src/OLED_IIC_Config.c'); Flags=$fastFlags}
        # The existing vendor font header uses flattened arrays and unused bitmaps.
        @{Name='oled_render_test'; Sources=@('tests/oled_render_test.c','Core/Src/OLED_Function.c','Core/Src/OLED_IIC_Config.c','Core/Src/power_menu.c'); Flags=@('-Wno-missing-braces','-Wno-unused-variable')}
    )
    foreach ($case in $cases) {
        $executable = Join-Path $PSScriptRoot ($case.Name + '.exe')
        $extraFlags = @()
        if ($case.ContainsKey('Flags')) { $extraFlags = @($case.Flags) }
        & gcc -std=c11 -O2 -Wall -Wextra -Werror @extraFlags -ICore/Inc -Itests @($case.Sources) -lm -o $executable
        if ($LASTEXITCODE -ne 0) { throw "$($case.Name) compile failed" }
        & $executable
        if ($LASTEXITCODE -ne 0) { throw "$($case.Name) failed" }
        Write-Output "$($case.Name): PASS"
    }
} finally {
    Pop-Location
}
