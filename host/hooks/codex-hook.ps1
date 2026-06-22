param(
    [ValidateSet(
        "UserPromptSubmit", "PreToolUse", "PermissionRequest",
        "PostToolUse", "Stop",
        "submitted", "thinking", "tool", "editing",
        "waiting", "blocked", "idle", "complete"
    )]
    [string]$Event = "complete",
    [string]$Session = "",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Rest
)

$ErrorActionPreference = "SilentlyContinue"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$inputText = if ([Console]::IsInputRedirected) {
    [Console]::In.ReadToEnd()
} else {
    ""
}
$payload = $null
$payloadText = $Rest |
    Where-Object { $_.Trim().StartsWith("{") } |
    Select-Object -First 1
if (-not $payloadText -and $inputText.Trim().StartsWith("{")) {
    $payloadText = $inputText
}
if ($payloadText) {
    $payload = $payloadText | ConvertFrom-Json
}
if (-not $Session) {
    $Session = $payload.session_id
}
if (-not $Session) {
    $Session = $payload.thread_id
}
if (-not $Session) {
    $Session = $payload.turn_id
}
if (-not $Session) {
    $Session = $payload.'turn-id'
}
if (-not $Session) {
    $Session = $payload.'thread-id'
}
if (-not $Session) {
    $Session = "codex-default"
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
$mappedEvent = switch ($Event) {
    "UserPromptSubmit" { "submitted" }
    "PreToolUse" {
        $toolName = $payload.tool_name
        if (-not $toolName) {
            $toolName = $payload.'tool-name'
        }
        if (-not $toolName) {
            $toolName = $payload.toolName
        }
        if ($toolName -match "^(apply_patch|Edit|Write)$") {
            "editing"
        } else {
            "tool"
        }
    }
    "PermissionRequest" { "waiting" }
    "PostToolUse" { "thinking" }
    "Stop" { "complete" }
    default { $Event }
}

if ($mappedEvent -eq "submitted") {
    $json = & $pythonCommand @pythonPrefix $stateScript begin `
        --source codex --session $Session
} elseif ($mappedEvent -eq "complete") {
    $eventText = "$($payload.type) $($payload.status) $($payload.outcome) $payloadText"
    $result = if ($eventText -match "error|fail|abort|cancel") {
        "failure"
    } else {
        "success"
    }
    $json = & $pythonCommand @pythonPrefix $stateScript complete --source codex `
        --session $Session --result $result
} else {
    $json = & $pythonCommand @pythonPrefix $stateScript status --source codex `
        --session $Session --state $mappedEvent
}
if ($json) {
    & $sender -Payload $json | Out-Null
}
exit 0
