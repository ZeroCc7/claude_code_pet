param(
    [Parameter(Mandatory = $true)]
    [string]$Event
)

$ErrorActionPreference = "SilentlyContinue"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$inputText = [Console]::In.ReadToEnd()
$payload = $null
if ($inputText.Trim().StartsWith("{")) {
    $payload = $inputText | ConvertFrom-Json
}
$session = if ($payload.session_id) {
    [string]$payload.session_id
} else {
    "claude-default"
}
$mapped = $Event
if ($Event -eq "PreToolUse") {
    $toolName = [string]$payload.tool_name
    if ($toolName -match "Edit|Write|NotebookEdit|MultiEdit") {
        $mapped = "editing"
    } elseif ($toolName -match "AskUserQuestion") {
        $mapped = "waiting"
    } else {
        $mapped = "tool"
    }
} elseif ($Event -match "PostToolUseFailure|PermissionDenied|StopFailure") {
    $mapped = "blocked"
} elseif ($Event -match "Notification|PermissionRequest") {
    $mapped = "waiting"
} elseif ($Event -eq "UserPromptSubmit") {
    $mapped = "submitted"
} elseif ($Event -eq "SessionEnd") {
    $mapped = "idle"
} elseif ($Event -eq "Stop") {
    $mapped = "complete"
}

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
if ($mapped -eq "submitted") {
    $json = & $pythonCommand @pythonPrefix $stateScript begin --source claude_code `
        --session $session
} elseif ($mapped -eq "complete") {
    $json = & $pythonCommand @pythonPrefix $stateScript complete --source claude_code `
        --session $session --result success
} else {
    $json = & $pythonCommand @pythonPrefix $stateScript status --source claude_code `
        --session $session --state $mapped
}
if ($json) {
    & $sender -Payload $json | Out-Null
}
exit 0
