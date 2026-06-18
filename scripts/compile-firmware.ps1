param(
    [string]$Cli = "$PSScriptRoot\..\tools\arduino-cli\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\.."
$sketch = Join-Path $root "firmware\ai_pet"
$build = Join-Path $root "build\firmware"

if (-not (Test-Path $Cli)) {
    throw "Arduino CLI not found: $Cli. Run scripts\bootstrap-arduino.ps1 first."
}

New-Item -ItemType Directory -Force $build | Out-Null

& $Cli compile `
    --fqbn "rp2040:rp2040:waveshare_rp2040_zero:flash=2097152_262144" `
    --warnings all `
    --output-dir $build `
    $sketch

if ($LASTEXITCODE -ne 0) {
    throw "Firmware compile failed with exit code $LASTEXITCODE"
}
