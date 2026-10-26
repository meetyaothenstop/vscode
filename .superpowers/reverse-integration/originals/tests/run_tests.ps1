$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    $cases = @(
        @{Name='adc_measure_test'; Sources=@('tests/adc_measure_test.c','Core/Src/adc_measure.c')},
        @{Name='adc_acquire_test'; Sources=@('tests/adc_acquire_test.c','Core/Src/adc_acquire.c','Core/Src/adc_measure.c')},
        @{Name='pwm_control_test'; Sources=@('tests/pwm_control_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c')}
        @{Name='pwm_switching_test'; Sources=@('tests/pwm_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c')}
        @{Name='power_menu_test'; Sources=@('tests/power_menu_test.c','Core/Src/power_menu.c')}
        @{Name='power_ui_test'; Sources=@('tests/power_ui_test.c','Core/Src/power_ui.c','Core/Src/power_menu.c')}
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
