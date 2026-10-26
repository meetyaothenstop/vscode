$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    $cases = @(
        @{Name='adc_measure_test'; Sources=@('tests/adc_measure_test.c','Core/Src/adc_measure.c')},
        @{Name='adc_acquire_test'; Sources=@('tests/adc_acquire_test.c','Core/Src/adc_acquire.c','Core/Src/adc_measure.c')},
        @{Name='pwm_control_test'; Sources=@('tests/pwm_control_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c')}
        @{Name='pwm_switching_test'; Sources=@('tests/pwm_switching_test.c','Core/Src/forward.c','Core/Src/pwm_plan.c')}
    )
    foreach ($case in $cases) {
        $executable = Join-Path $PSScriptRoot ($case.Name + '.exe')
        & gcc -std=c11 -O2 -Wall -Wextra -Werror -ICore/Inc -Itests @($case.Sources) -lm -o $executable
        if ($LASTEXITCODE -ne 0) { throw "$($case.Name) compile failed" }
        & $executable
        if ($LASTEXITCODE -ne 0) { throw "$($case.Name) failed" }
        Write-Output "$($case.Name): PASS"
    }
} finally {
    Pop-Location
}
