param(
    [string]$Port = "COM7"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $projectRoot "host\hooks"
$installRoot = Join-Path $env:USERPROFILE ".ai-pet-hooks"
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"

New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
Copy-Item -Path (Join-Path $sourceRoot "*") -Destination $installRoot `
    -Recurse -Force
[Environment]::SetEnvironmentVariable("AI_PET_PORT", $Port, "User")
@{ port = $Port } | ConvertTo-Json |
    Set-Content -LiteralPath (Join-Path $installRoot "config.json") `
        -Encoding UTF8

function Add-ClaudeHook($Hooks, [string]$Name, [string]$Command) {
    $entry = [pscustomobject]@{
        hooks = @(
            [pscustomobject]@{
                type = "command"
                command = $Command
                timeout = 5
            }
        )
    }
    if ($Hooks.PSObject.Properties.Name -contains $Name) {
        $existing = @($Hooks.$Name)
        $serialized = $existing | ConvertTo-Json -Depth 10
        if ($serialized -notmatch "\.ai-pet-hooks") {
            $Hooks.$Name = @($existing + $entry)
        }
    } else {
        $Hooks | Add-Member -NotePropertyName $Name `
            -NotePropertyValue @($entry)
    }
}

$claudeSettings = Join-Path $env:USERPROFILE ".claude\settings.json"
if (Test-Path -LiteralPath $claudeSettings) {
    Copy-Item -LiteralPath $claudeSettings `
        "$claudeSettings.ai-pet-$timestamp.bak"
    $settings = Get-Content -LiteralPath $claudeSettings -Raw |
        ConvertFrom-Json
    if (-not ($settings.PSObject.Properties.Name -contains "hooks")) {
        $settings | Add-Member -NotePropertyName hooks `
            -NotePropertyValue ([pscustomobject]@{})
    }
    $claudeScript = Join-Path $installRoot "claude-hook.ps1"
    foreach ($event in @(
        "UserPromptSubmit", "PreToolUse", "PostToolUseFailure",
        "PermissionRequest", "Notification", "Stop", "SessionEnd"
    )) {
        $command = "powershell -NoProfile -ExecutionPolicy Bypass -File " +
            "`"$claudeScript`" -Event $event"
        Add-ClaudeHook $settings.hooks $event $command
    }
    $settings | ConvertTo-Json -Depth 20 |
        Set-Content -LiteralPath $claudeSettings -Encoding UTF8
    Write-Host "Claude Code hooks installed."
}

$cursorLightNotify = Join-Path $env:USERPROFILE `
    ".codex\cursor-light\codex-notify.ps1"
if (Test-Path -LiteralPath $cursorLightNotify) {
    $content = Get-Content -LiteralPath $cursorLightNotify -Raw
    if ($content -notmatch "AI-PET-HOOK-BEGIN") {
        Copy-Item -LiteralPath $cursorLightNotify `
            "$cursorLightNotify.ai-pet-$timestamp.bak"
        $codexScript = Join-Path $installRoot "codex-hook.ps1"
        $block = @"

# AI-PET-HOOK-BEGIN
try {
    & "$codexScript" -Event complete @Rest
} catch {
}
# AI-PET-HOOK-END
"@
        Add-Content -LiteralPath $cursorLightNotify -Value $block
    }
    Write-Host "Codex completion hook chained after CursorLight."
} else {
    Write-Warning "CursorLight Codex notifier not found; see docs/ai-hooks-guide.md."
}

if (Get-Command opencode -ErrorAction SilentlyContinue) {
    $opencodePlugins = Join-Path $env:USERPROFILE ".config\opencode\plugins"
    New-Item -ItemType Directory -Force -Path $opencodePlugins | Out-Null
    Copy-Item -LiteralPath (Join-Path $installRoot "opencode-plugin.js") `
        -Destination (Join-Path $opencodePlugins "ai-pet.js") -Force
    Write-Host "OpenCode plugin installed."
} else {
    Write-Host "OpenCode not detected; plugin template was not installed."
}

Write-Host "AI pet hooks installed at $installRoot"
Write-Host "Port: $Port"
