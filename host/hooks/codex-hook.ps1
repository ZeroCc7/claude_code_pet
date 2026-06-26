param(
    [ValidateSet("start", "end")]
    [string]$Event
)

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$pythonCommand = (Get-Command py -ErrorAction SilentlyContinue).Source
$pythonPrefix = @("-3")
if (-not $pythonCommand) {
    $pythonCommand = (Get-Command python -ErrorAction SilentlyContinue).Source
    $pythonPrefix = @()
}
if ($pythonCommand) {
    & $pythonCommand @pythonPrefix (Join-Path $root "ai_pet_hook.py") `
        $Event --source codex | Out-Null
}
exit 0
