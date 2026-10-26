$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$mdkRoot = Join-Path $projectRoot 'MDK-ARM'
$utf8 = [Text.UTF8Encoding]::new($false)
$project = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_ccm_base.uvprojx'))
$project = $project.Replace('bench12_ccm_base','bench12_buck_fast')
if ([regex]::Matches($project,'POWER_CCM_CONTROL=1').Count -ne 1) { throw 'Unexpected CCM macro list' }
$project = $project.Replace('POWER_CCM_CONTROL=1','POWER_CCM_CONTROL=1,POWER_BENCH_FAST_BUCK=1')
[xml]$parsed = $project
$defines = $parsed.SelectSingleNode('//Cads/VariousControls/Define').InnerText
foreach ($required in @('POWER_BENCH_12V_BUCK_TEST=1','POWER_TEST_SKIP_OLED_ACTIVE=1',
    'POWER_UNIFIED_DIAGNOSTICS=0','POWER_CCM_CONTROL=1','POWER_BENCH_FAST_BUCK=1','POWER_OCP_WIRING_CONFIRMED=0')) {
    if (!$defines.Split(',').Contains($required)) { throw "Missing restriction: $required" }
}
if ($parsed.SelectSingleNode('//LDads/ScatterFile').InnerText -ne '.\bench12_ccm_diag.sct') { throw 'Wrong CCM scatter' }
foreach ($tag in @('RunUserProg1','RunUserProg2')) {
    foreach ($node in $parsed.SelectNodes("//$tag")) {
        if ($node.InnerText -ne '0') { throw 'Unexpected enabled build hook' }
    }
}
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_buck_fast.uvprojx'),$project,$utf8)
[xml]$options = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_ccm_base.uvoptx')).Replace('bench12_ccm_base','bench12_buck_fast')
# Do not carry an interactive user's breakpoints into the new power test image.
foreach ($node in @($options.SelectNodes('//Breakpoint'))) { [void]$node.ParentNode.RemoveChild($node) }
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_buck_fast.uvoptx'),$options.OuterXml,$utf8)
Write-Output 'Prepared restricted bench12_buck_fast; startup <0.5 A / <=1 V; original timing guards.'
