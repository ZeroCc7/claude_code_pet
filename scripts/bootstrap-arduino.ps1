$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\.."
$toolDir = Join-Path $root "tools\arduino-cli"
$zipPath = Join-Path $env:TEMP "arduino-cli-windows.zip"
$cli = Join-Path $toolDir "arduino-cli.exe"

function Invoke-ArduinoCli {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments)

    & $cli @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "arduino-cli failed: $($Arguments -join ' ')"
    }
}

New-Item -ItemType Directory -Force $toolDir | Out-Null

if (-not (Test-Path $cli)) {
    $release = Invoke-RestMethod `
        -Headers @{ "User-Agent" = "claude-code-pet-bootstrap" } `
        "https://api.github.com/repos/arduino/arduino-cli/releases/latest"
    $asset = $release.assets |
        Where-Object { $_.name -match '^arduino-cli_.*_Windows_64bit\.zip$' } |
        Select-Object -First 1

    if (-not $asset) {
        throw "Arduino CLI Windows asset not found"
    }

    Invoke-WebRequest $asset.browser_download_url -OutFile $zipPath
    Expand-Archive $zipPath $toolDir -Force
}

Invoke-ArduinoCli config init --overwrite
Invoke-ArduinoCli config add board_manager.additional_urls `
    "https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json"

$internetSettings = Get-ItemProperty `
    "HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings" `
    -ErrorAction SilentlyContinue
if ($internetSettings.ProxyEnable -eq 1 -and $internetSettings.ProxyServer) {
    $proxy = [string]$internetSettings.ProxyServer
    if ($proxy -notmatch "^[a-z]+://") {
        $proxy = "http://$proxy"
    }
    Invoke-ArduinoCli config set network.proxy $proxy
}

Invoke-ArduinoCli core update-index
Invoke-ArduinoCli core install "rp2040:rp2040"
Invoke-ArduinoCli lib install "Adafruit GFX Library"
Invoke-ArduinoCli lib install "Adafruit ST7735 and ST7789 Library"
Invoke-ArduinoCli lib install "U8g2_for_Adafruit_GFX"
Invoke-ArduinoCli version
Invoke-ArduinoCli core list
Invoke-ArduinoCli lib list
