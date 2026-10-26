$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$mdkRoot = Join-Path $projectRoot 'MDK-ARM'
$utf8 = [Text.UTF8Encoding]::new($false)
$original = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_buck_oled_test.uvprojx'))
$project = $original.Replace('bench12_oled_mute', 'bench12_unified_diag')
if ([regex]::Matches($project, 'POWER_TEST_SKIP_OLED_ACTIVE=1').Count -ne 1) {
    throw 'Expected one OLED-mute compiler definition'
}
$project = $project.Replace('POWER_TEST_SKIP_OLED_ACTIVE=1',
    'POWER_TEST_SKIP_OLED_ACTIVE=1,POWER_UNIFIED_DIAGNOSTICS=1')
$project = [regex]::Replace($project, '(<RunUserProg[12]>)1(</RunUserProg[12]>)', '${1}0${2}')
$targetPath = Join-Path $mdkRoot 'bench12_unified_diag.uvprojx'
[IO.File]::WriteAllText($targetPath, $project, $utf8)

$options = [IO.File]::ReadAllText((Join-Path $mdkRoot 'bench12_buck_oled_test.uvoptx'))
$options = $options.Replace('bench12_oled_mute', 'bench12_unified_diag')
$expressions = @('power_status.fault', 'power_status.fault_detail', 'power_diag.fault',
    'power_diag.fault.profile', 'power_diag.write_budget_cycles',
    'power_diag.exit_budget_cycles', 'power_status.isr_cycles_max')
$watchRows = for ($watchIndex=0; $watchIndex -lt $expressions.Count; ++$watchIndex) {
    '        <Ww><count>{0}</count><WinNumber>1</WinNumber><ItemText>{1}</ItemText></Ww>' -f $watchIndex,$expressions[$watchIndex]
}
$watch = "<WatchWindow1>`n" + ($watchRows -join "`n") + "`n      </WatchWindow1>"
if ([regex]::Matches($options, '(?s)<WatchWindow1>.*?</WatchWindow1>').Count -ne 1) {
    throw 'Expected one Watch1 configuration'
}
$options = [regex]::Replace($options, '(?s)<WatchWindow1>.*?</WatchWindow1>', $watch)
$options = [regex]::Replace($options, '(?s)<Breakpoint>.*?</Breakpoint>', '<Breakpoint/>')
[IO.File]::WriteAllText((Join-Path $mdkRoot 'bench12_unified_diag.uvoptx'), $options, $utf8)

# Private build-only comparison. Convert paths relative to the original MDK
# directory so this project can live beside the ledger, away from delivery.
[xml]$comparison = $original
foreach ($node in $comparison.SelectNodes('//TargetName')) { $node.InnerText = 'unified_compare' }
foreach ($tag in @('FilePath','IncludePath','ScatterFile')) {
    foreach ($node in $comparison.SelectNodes("//$tag")) {
        if (!$node.InnerText) { continue }
        $paths = $node.InnerText.Split(';')
        $node.InnerText = (($paths | ForEach-Object {
            if ($_.Contains('$') -or [IO.Path]::IsPathRooted($_)) { $_ }
            else { [IO.Path]::GetFullPath((Join-Path $mdkRoot $_)) }
        }) -join ';')
    }
}
$privateOutput = Join-Path $PSScriptRoot 'arm_compare'
New-Item -ItemType Directory -Path $privateOutput -Force | Out-Null
foreach ($tag in @('OutputDirectory','ListingPath')) {
    foreach ($node in $comparison.SelectNodes("//$tag")) { $node.InnerText = $privateOutput + '\' }
}
foreach ($node in $comparison.SelectNodes('//OutputName')) { $node.InnerText = 'bidreaction_unified_compare' }
foreach ($tag in @('RunUserProg1','RunUserProg2')) {
    foreach ($node in $comparison.SelectNodes("//$tag")) { $node.InnerText = '0' }
}
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'unified_compare.uvprojx'), $comparison.OuterXml, $utf8)

[xml]$parsed = $project
if ($parsed.SelectSingleNode('//TargetName').InnerText -ne 'bench12_unified_diag' -or
    !$parsed.SelectSingleNode('//Cads/VariousControls/Define').InnerText.Contains('POWER_UNIFIED_DIAGNOSTICS=1') -or
    $project.Contains('bench12_oled_mute')) { throw 'Diagnostic target identity mismatch' }
foreach ($tag in @('RunUserProg1','RunUserProg2')) {
    foreach ($node in $parsed.SelectNodes("//$tag")) {
        if ($node.InnerText -ne '0') { throw 'User program hook still enabled' }
    }
}
Write-Output $targetPath
