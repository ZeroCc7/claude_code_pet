param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("submitted", "thinking", "tool", "editing",
                 "waiting", "blocked", "idle", "complete")]
    [string]$Event,
    [string]$Session = "opencode-default"
)

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$pythonCommand = (Get-Command py -ErrorAction SilentlyContinue).Source
$pythonPrefix = @("-3")
if (-not $pythonCommand) {
    $pythonCommand = (Get-Command python -ErrorAction SilentlyContinue).Source
    $pythonPrefix = @()
}
if (-not $pythonCommand) {
    exit 0
}
$stateScript = Join-Path $root "hook_state.py"
$sender = Join-Path $root "send-ai-pet-event.ps1"
if ($Event -eq "submitted") {
    $json = & $pythonCommand @pythonPrefix $stateScript begin --source opencode `
        --session $Session
} elseif ($Event -eq "complete") {
    $json = & $pythonCommand @pythonPrefix $stateScript complete --source opencode `
        --session $Session --result success
} else {
    $json = & $pythonCommand @pythonPrefix $stateScript status --source opencode `
        --session $Session --state $Event
}
if ($json) {
    & $sender -Payload $json | Out-Null
}
exit 0
