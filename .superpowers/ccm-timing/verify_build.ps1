param(
    [Parameter(Mandatory=$true)][string]$MapPath,
    [Parameter(Mandatory=$true)][string]$HexPath,
    [switch]$Uninstrumented
)
$ErrorActionPreference = 'Stop'
$mapText = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $MapPath))
function Get-SymbolAddress([string]$Symbol) {
    $match = [regex]::Match($mapText, '(?m)^\s+' + [regex]::Escape($Symbol) + '\s+0x([0-9a-fA-F]+)\b')
    if (!$match.Success) { throw "Missing linked symbol: $Symbol" }
    return [Convert]::ToUInt32($match.Groups[1].Value,16)
}
$region = [regex]::Match($mapText,
    'Execution Region ER_CCMRAM \(Exec base:\s*0x([0-9a-fA-F]+), Load base:\s*0x([0-9a-fA-F]+), Size:\s*0x([0-9a-fA-F]+), Max:\s*0x([0-9a-fA-F]+)')
if (!$region.Success) { throw 'Missing ER_CCMRAM execution region' }
$execBase = [Convert]::ToUInt32($region.Groups[1].Value,16)
$loadBase = [Convert]::ToUInt32($region.Groups[2].Value,16)
$codeSize = [Convert]::ToUInt32($region.Groups[3].Value,16)
$maxSize = [Convert]::ToUInt32($region.Groups[4].Value,16)
if ($execBase -ne 0x10000000 -or $maxSize -ne 0x8000 -or $codeSize -eq 0 -or
    $codeSize -gt 0x8000 -or $loadBase -lt 0x08000000 -or
    ($loadBase + $codeSize) -gt 0x08020000) { throw 'Unsafe CCM execution/load bounds' }
$hotSymbols = @('HRTIM1_TIMB_IRQHandler','Power_TimerIRQ','enable_outputs',
    'forward_step','PWM_MakePlan','PWM_EntryIsTimely','PowerProtection_FaultPending')
foreach ($symbol in $hotSymbols) {
    $address = Get-SymbolAddress $symbol
    if ($address -lt $execBase -or $address -ge ($execBase + $codeSize)) {
        throw "Hot symbol did not move into CCM: $symbol"
    }
}
foreach ($symbol in @('Reset_Handler','Power_EmergencyOff','HardFault_Handler','__scatterload_copy')) {
    $address = Get-SymbolAddress $symbol
    if ($address -lt 0x08000000 -or $address -ge 0x08020000) {
        throw "Early boot/emergency code not executable in Flash: $symbol"
    }
}
if ($mapText -notmatch 'Execution Region RW_IRAM1 \(Exec base:\s*0x20000000,.*?Max:\s*0x00018000') {
    throw 'RW region does not exclude the CCM alias'
}
$ramSymbols = @('power_status','__initial_sp')
if ($Uninstrumented) {
    if ($mapText -match '(?m)^\s+(?:power_diag|diag_[A-Za-z_]\w*)\s+0x') {
        throw 'Diagnostic probe/storage symbols still linked into uninstrumented image'
    }
} else { $ramSymbols += 'power_diag' }
foreach ($symbol in $ramSymbols) {
    $address = Get-SymbolAddress $symbol
    if ($address -lt 0x20000000 -or $address -ge 0x20018000) {
        throw "RAM data/stack may overlap CCM code alias: $symbol"
    }
}

# Decode the actual Flash download image and inspect its scatter-copy entry
# and IRQ vector. This checks linked behavior rather than scatter-file text.
$image = [Collections.Generic.Dictionary[uint32,byte]]::new()
[uint32]$upper = 0
foreach ($line in [IO.File]::ReadAllLines((Resolve-Path -LiteralPath $HexPath))) {
    if (!$line.StartsWith(':')) { throw 'Invalid HEX record' }
    $bytes = for ($index=1; $index -lt $line.Length; $index+=2) {
        [Convert]::ToByte($line.Substring($index,2),16)
    }
    $sum = 0; foreach ($value in $bytes) { $sum += $value }
    if (($sum -band 0xFF) -ne 0 -or $bytes.Count -ne ($bytes[0]+5)) { throw 'Bad HEX checksum/length' }
    $type = $bytes[3]
    if ($type -eq 4) { $upper = [uint32](($bytes[4]*256+$bytes[5])*65536); continue }
    if ($type -eq 1) { break }
    if ($type -eq 5) { continue }
    if ($type -ne 0) { throw "Unexpected HEX record type: $type" }
    $recordBase = $upper + $bytes[1]*256 + $bytes[2]
    for ($index=0; $index -lt $bytes[0]; ++$index) {
        $address = [uint32]($recordBase+$index)
        if ($address -lt 0x08000000 -or $address -ge 0x08020000) { throw 'Download contains non-Flash records' }
        $image[$address] = $bytes[$index+4]
    }
}
function Read-U32([uint32]$Address) {
    [uint32]$value = 0
    for ($byteIndex=0; $byteIndex -lt 4; ++$byteIndex) {
        [uint32]$key = $Address+$byteIndex
        if (!$image.ContainsKey($key)) { throw ('Missing Flash image byte at0x{0:X8}' -f $key) }
        $value = $value -bor ([uint32]$image[$key] -shl (8*$byteIndex))
    }
    return $value
}
$vector = Read-U32 (0x08000000 + (16+69)*4)
if ($vector -ne ((Get-SymbolAddress 'HRTIM1_TIMB_IRQHandler') -bor 1)) { throw 'IRQ vector does not point to CCM wrapper' }
if ((Read-U32 0x08000004) -ne ((Get-SymbolAddress 'Reset_Handler') -bor 1)) { throw 'Reset vector mismatch' }
$tableBase = Get-SymbolAddress 'Region$$Table$$Base'
$tableLimit = Get-SymbolAddress 'Region$$Table$$Limit'
$copyHandler = Get-SymbolAddress '__scatterload_copy'
$foundCopy = $false
for ($entry=$tableBase; ($entry+16) -le $tableLimit; $entry+=16) {
    if ((Read-U32 ($entry+4)) -eq $execBase) {
        if ((Read-U32 $entry) -ne $loadBase -or (Read-U32 ($entry+8)) -ne $codeSize -or
            ((Read-U32 ($entry+12)) -bor 1) -ne ($copyHandler -bor 1)) {
            throw 'CCM startup copy is missing or compressed/unexpected'
        }
        $foundCopy = $true
    }
}
if (!$foundCopy) { throw 'No startup copy entry for CCM execution region' }
for ($address=$loadBase; $address -lt ($loadBase+$codeSize); ++$address) {
    if (!$image.ContainsKey([uint32]$address)) { throw 'Incomplete CCM load image in Flash HEX' }
}
Write-Output ('CCM placement/copy/vector PASS: {0} bytes at0x{1:X8}, loaded from0x{2:X8}' -f $codeSize,$execBase,$loadBase)
