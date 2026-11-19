<#
.SYNOPSIS
  Build (and optionally run) OpenGlCore.

.EXAMPLE
  ./build.ps1                 # build Debug x64
  ./build.ps1 -Run            # build Debug x64, then run
  ./build.ps1 -Config Release -Run
  ./build.ps1 -Rebuild        # clean rebuild
#>
param(
    [ValidateSet('Debug', 'Release')] [string]$Config = 'Debug',
    [ValidateSet('x64', 'Win32')]     [string]$Platform = 'x64',
    [switch]$Run,
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# --- Locate MSBuild via vswhere ---
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found. Is Visual Studio installed?" }
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild.exe not found via vswhere." }

# --- Build ---
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
Write-Host "Building $Config|$Platform ($target)..." -ForegroundColor Cyan
& $msbuild "$root\OpenGlCore.sln" "/p:Configuration=$Config" "/p:Platform=$Platform" "/t:$target" /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE)." }

$exe = Join-Path $root "$Platform\$Config\OpenGlCore.exe"
Write-Host "Build succeeded -> $exe" -ForegroundColor Green

# --- Run (with the correct working directory for relative asset paths) ---
if ($Run) {
    if (-not (Test-Path $exe)) { throw "Executable not found: $exe" }
    $cwd = Join-Path $root 'OpenGlCore'   # must be where Shader/ and Images/ live
    Write-Host "Running (cwd: $cwd)..." -ForegroundColor Cyan
    Push-Location $cwd
    try { & $exe } finally { Pop-Location }
}
