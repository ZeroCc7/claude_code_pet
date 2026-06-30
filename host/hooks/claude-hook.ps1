param(
    [ValidateSet("UserPromptSubmit", "Stop")]
    [string]$Event
)

$ErrorActionPreference = "SilentlyContinue"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$pythonCommand = (Get-Command py -ErrorAction SilentlyContinue).Source
$pythonPrefix = @("-3")
if (-not $pythonCommand) {
    $pythonCommand = (Get-Command python -ErrorAction SilentlyContinue).Source
    $pythonPrefix = @()
}
if ($pythonCommand) {
    $command = if ($Event -eq "UserPromptSubmit") { "start" } else { "end" }
    & $pythonCommand @pythonPrefix (Join-Path $root "ai_pet_hook.py") `
        $command --source claude-code | Out-Null
}
exit 0
