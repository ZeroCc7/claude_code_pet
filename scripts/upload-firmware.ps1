param(
    [Parameter(Mandatory = $true)]
    [string]$Port,
    [string]$Cli = "$PSScriptRoot\..\tools\arduino-cli\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\.."
$sketch = Join-Path $root "firmware\ai_pet"

if (-not (Test-Path $Cli)) {
    throw "Arduino CLI not found: $Cli. Run scripts\bootstrap-arduino.ps1 first."
}

& $Cli upload `
    --fqbn "rp2040:rp2040:waveshare_rp2040_zero" `
    --port $Port `
    $sketch

if ($LASTEXITCODE -ne 0) {
    throw "Firmware upload failed with exit code $LASTEXITCODE"
}

