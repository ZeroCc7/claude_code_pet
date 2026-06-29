param(
    [ValidateSet("UserPromptSubmit", "Stop", "start", "end")]
    [string]$Event
)

$command = switch ($Event) {
    "UserPromptSubmit" { "start" }
    "Stop" { "end" }
    default { $Event }
}

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$pythonCommand = (Get-Command py -ErrorAction SilentlyContinue).Source
$pythonPrefix = @("-3")
if (-not $pythonCommand) {
    $pythonCommand = (Get-Command python -ErrorAction SilentlyContinue).Source
    $pythonPrefix = @()
}
if ($pythonCommand) {
    & $pythonCommand @pythonPrefix (Join-Path $root "ai_pet_hook.py") `
        $command --source codex | Out-Null
}
exit 0
