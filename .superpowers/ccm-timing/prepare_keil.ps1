$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$mdkRoot = Join-Path $projectRoot 'MDK-ARM'
$utf8 = [Text.UTF8Encoding]::new($false)
$project = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_unified_diag.uvprojx'))
$project = $project.Replace('bench12_unified_diag','bench12_ccm_diag')
if ([regex]::Matches($project,'POWER_UNIFIED_DIAGNOSTICS=1').Count -ne 1) { throw 'Unexpected macro list' }
$project = $project.Replace('POWER_UNIFIED_DIAGNOSTICS=1','POWER_UNIFIED_DIAGNOSTICS=1,POWER_CCM_CONTROL=1')
$project = $project.Replace('<umfTarg>1</umfTarg>','<umfTarg>0</umfTarg>')
$project = $project.Replace('<useFile>0</useFile>','<useFile>1</useFile>')
$project = $project.Replace('<ScatterFile></ScatterFile>','<ScatterFile>.\bench12_ccm_diag.sct</ScatterFile>')
$project = $project.Replace('<Misc></Misc>','<Misc>--datacompressor=off</Misc>')
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_ccm_diag.uvprojx'),$project,$utf8)
$options = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_unified_diag.uvoptx'))
$options = $options.Replace('bench12_unified_diag','bench12_ccm_diag')
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_ccm_diag.uvoptx'),$options,$utf8)
[xml]$parsed = $project
if ($parsed.SelectSingleNode('//LDads/umfTarg').InnerText -ne '0' -or
    $parsed.SelectSingleNode('//LDads/ScatterFile').InnerText -ne '.\bench12_ccm_diag.sct') {
    throw 'Custom CCM scatter file is not selected'
}
foreach ($tag in @('RunUserProg1','RunUserProg2')) {
    foreach ($node in $parsed.SelectNodes("//$tag")) {
        if ($node.InnerText -ne '0') { throw 'Unexpected enabled build hook' }
    }
}
Write-Output 'Prepared bench12_ccm_diag (build-only; no download/start).'
