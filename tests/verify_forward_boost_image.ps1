param(
    [Parameter(Mandatory=$true)][string]$MapPath,
    [Parameter(Mandatory=$true)][string]$DisassemblyPath,
    [string]$BoostSymbol = 'forward_main_boost_step',
    [switch]$BudgetOnly
)
$ErrorActionPreference = 'Stop'
$mapText = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $MapPath))
$assemblyLines = [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $DisassemblyPath))
function Read-Function([string]$Name) {
    $m = [regex]::Match($mapText, '(?m)^\s+' + [regex]::Escape($Name) + '\s+0x([0-9a-fA-F]+)\s+Thumb Code\s+(\d+)\b')
    if (!$m.Success) { throw "Missing ARM symbol: $Name" }
    $start = [Convert]::ToUInt32($m.Groups[1].Value, 16) -band 0xFFFFFFFE
    $size = [int]$m.Groups[2].Value
    $lines = @($assemblyLines | Where-Object {
        if ($_ -match '^\s*0x([0-9a-fA-F]+):') {
            $a = [Convert]::ToUInt32($Matches[1], 16)
            $a -ge $start -and $a -lt ($start + $size)
        } else { $false }
    })
    if (!$lines.Count) { throw "Missing disassembled instructions: $Name" }
    [pscustomobject]@{Name=$Name; Address=$start; Bytes=$size; Lines=$lines}
}
$boost = Read-Function $BoostSymbol
$divisions = @($boost.Lines | Where-Object { $_ -cmatch '\bVDIV\.F32\b' }).Count
if ($divisions -ne 1) { throw "$BoostSymbol has $divisions VDIV.F32 instructions; protected Boost budget is exactly1" }
if ($BudgetOnly) { Write-Output "$BoostSymbol division budget PASS:1"; exit 0 }
$full = Read-Function 'forward_main_full_step'
if ($boost.Bytes -ge $full.Bytes) { throw 'Protected Boost step is not smaller than the full compatibility step' }
$region = [regex]::Match($mapText, 'Execution Region ER_CCMRAM \(Exec base:\s*0x([0-9a-fA-F]+), Load base:\s*0x[0-9a-fA-F]+, Size:\s*0x([0-9a-fA-F]+)')
if (!$region.Success) { throw 'CCM region missing' }
$ccmStart = [Convert]::ToUInt32($region.Groups[1].Value, 16)
$ccmSize = [Convert]::ToUInt32($region.Groups[2].Value, 16)
$worker = Read-Function 'Power_ControlIRQ'
foreach ($f in @($boost, $worker)) {
    if ($f.Address -lt $ccmStart -or ($f.Address + $f.Bytes) -gt ($ccmStart + $ccmSize)) {
        throw ($f.Name + ' is not wholly in CCM')
    }
}
if (!@($worker.Lines | Where-Object { $_ -cmatch '\bBL(?:\.W)?\s+forward_main_boost_step\b' }).Count) {
    throw 'Real worker does not call the protected Boost step'
}
foreach ($name in @('Power_TimerIRQ','raw_checks','control_slice','control_write','control_modulate')) {
    $f = Read-Function $name
    if (@($f.Lines | Where-Object { $_ -cmatch '^\s*0x[0-9a-fA-F]+:\s+[0-9a-fA-F]+\s+\S+\s+V[A-Z0-9.]+\s' }).Count) {
        throw "Fast IRQ function contains FP instructions: $name"
    }
}
Write-Output ('Protected Boost ARM image PASS: bytes={0}, full={1}, VDIV={2}; worker/CCM/fast integer paths verified' -f $boost.Bytes,$full.Bytes,$divisions)
