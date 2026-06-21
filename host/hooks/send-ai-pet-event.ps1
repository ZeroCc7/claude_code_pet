param(
    [Parameter(Mandatory = $true)]
    [string]$Payload,
    [string]$Port = $env:AI_PET_PORT,
    [int]$TimeoutMs = 1200
)

$ErrorActionPreference = "Stop"
$installRoot = Join-Path $env:USERPROFILE ".ai-pet-hooks"
$logPath = Join-Path $installRoot "ai-pet-hooks.log"
New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
if (-not $Port) {
    $configPath = Join-Path $installRoot "config.json"
    if (Test-Path -LiteralPath $configPath) {
        try {
            $Port = (Get-Content -LiteralPath $configPath -Raw |
                ConvertFrom-Json).port
        } catch {
        }
    }
}

function Write-HookLog([string]$Message) {
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss.fff"
    Add-Content -LiteralPath $logPath -Value "[$timestamp] $Message"
}

function Find-AiPetPort {
    if ($Port) {
        return $Port
    }
    try {
        $candidate = Get-CimInstance Win32_PnPEntity -ErrorAction Stop |
            Where-Object {
                $_.Name -match '\(COM\d+\)' -and
                ($_.PNPDeviceID -match 'VID_2E8A&PID_0003' -or
                 $_.Name -match 'RP2040|Pico|USB Serial|USB CDC')
            } |
            Select-Object -First 1
        if ($candidate -and $candidate.Name -match '(COM\d+)') {
            return $Matches[1]
        }
    } catch {
        Write-HookLog "port discovery failed: $($_.Exception.Message)"
    }
    return $null
}

$mutex = [Threading.Mutex]::new($false, "Global\ClaudeCodePetSerial")
$locked = $false
$serial = $null
try {
    $locked = $mutex.WaitOne(1500)
    if (-not $locked) {
        Write-HookLog "skip: serial mutex timeout"
        exit 0
    }
    $selectedPort = Find-AiPetPort
    if (-not $selectedPort) {
        Write-HookLog "skip: RP2040 port not found"
        exit 0
    }

    $serial = [IO.Ports.SerialPort]::new(
        $selectedPort, 115200, "None", 8, "One"
    )
    $serial.NewLine = "`n"
    $serial.ReadTimeout = 200
    $serial.WriteTimeout = 500
    $serial.DtrEnable = $true
    $serial.Open()
    Start-Sleep -Milliseconds 80
    $serial.DiscardInBuffer()
    $serial.WriteLine($Payload)
    $ack = ""
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while ($watch.ElapsedMilliseconds -lt $TimeoutMs) {
        try {
            $line = $serial.ReadLine().Trim()
            if ($line -match '"type"\s*:\s*"ack"') {
                $ack = $line
                break
            }
        } catch [TimeoutException] {
        }
    }
    Write-HookLog "port=$selectedPort payload=$Payload ack=$ack"
    if ($ack) {
        Write-Output $ack
    }
} catch {
    Write-HookLog "send failed: $($_.Exception.Message)"
} finally {
    if ($serial -and $serial.IsOpen) {
        $serial.Close()
    }
    if ($locked) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}

exit 0
