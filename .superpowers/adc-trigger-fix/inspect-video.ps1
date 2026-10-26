param([Parameter(Mandatory=$true)][string]$VideoPath,
      [Parameter(Mandatory=$true)][string]$OutputDir)
$ErrorActionPreference = 'Stop'
$toolFolder = Join-Path $PSScriptRoot '..\video-tools\imageio_ffmpeg\binaries'
$executables = @(Get-ChildItem -LiteralPath $toolFolder -Filter '*.exe')
if ($executables.Count -ne 1) { throw 'Expected one installed FFmpeg executable' }
[void][System.IO.Directory]::CreateDirectory($OutputDir)
& $executables[0].FullName -hide_banner -nostdin -n -i $VideoPath -vf 'fps=1' -frames:v 30 (Join-Path $OutputDir 'video-%03d.png')
if ($LASTEXITCODE -ne 0) { throw 'Video extraction failed; use an empty output directory and verify access permissions' }