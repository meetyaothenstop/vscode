$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$mdkRoot = Join-Path $projectRoot 'MDK-ARM'
$utf8 = [Text.UTF8Encoding]::new($false)
$project = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_ccm_diag.uvprojx'))
$project = $project.Replace('bench12_ccm_diag','bench12_ccm_base')
$project = $project.Replace('<ScatterFile>.\bench12_ccm_base.sct</ScatterFile>',
    '<ScatterFile>.\bench12_ccm_diag.sct</ScatterFile>')
if ([regex]::Matches($project,'POWER_UNIFIED_DIAGNOSTICS=1').Count -ne 1) { throw 'Unexpected profiler macro list' }
$project = $project.Replace('POWER_UNIFIED_DIAGNOSTICS=1','POWER_UNIFIED_DIAGNOSTICS=0')
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_ccm_base.uvprojx'),$project,$utf8)
$options = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_ccm_diag.uvoptx'))
$options = $options.Replace('bench12_ccm_diag','bench12_ccm_base')
$expressions = @('power_status.fault','power_status.fault_detail',
    'power_status.fault_elapsed_cycles','power_status.fault_spacing_cycles',
    'power_status.fault_counter_ticks','power_status.isr_cycles_max',
    'power_status.v_reference','power_status.fault_v_right','power_status.fault_il','power_status')
$watchRows = for ($index=0; $index -lt $expressions.Count; ++$index) {
    '        <Ww><count>{0}</count><WinNumber>1</WinNumber><ItemText>{1}</ItemText></Ww>' -f $index,$expressions[$index]
}
$watch = "<WatchWindow1>`n" + ($watchRows -join "`n") + "`n      </WatchWindow1>"
$options = [regex]::Replace($options,'(?s)<WatchWindow1>.*?</WatchWindow1>',$watch)
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_ccm_base.uvoptx'),$options,$utf8)
[xml]$parsed = $project
$defines = $parsed.SelectSingleNode('//Cads/VariousControls/Define').InnerText
foreach ($required in @('POWER_BENCH_12V_BUCK_TEST=1','POWER_TEST_SKIP_OLED_ACTIVE=1',
    'POWER_UNIFIED_DIAGNOSTICS=0','POWER_CCM_CONTROL=1','POWER_OCP_WIRING_CONFIRMED=0')) {
    if (!$defines.Split(',').Contains($required)) { throw "Missing restriction/configuration: $required" }
}
if ($parsed.SelectSingleNode('//LDads/umfTarg').InnerText -ne '0' -or
    $parsed.SelectSingleNode('//LDads/ScatterFile').InnerText -ne '.\bench12_ccm_diag.sct' -or
    $options.Contains('power_diag')) { throw 'Baseline scatter/Watch mismatch' }
foreach ($tag in @('RunUserProg1','RunUserProg2')) {
    foreach ($node in $parsed.SelectNodes("//$tag")) {
        if ($node.InnerText -ne '0') { throw 'Unexpected enabled build hook' }
    }
}
Write-Output 'Prepared bench12_ccm_base: unified probes0; protection unchanged.'
