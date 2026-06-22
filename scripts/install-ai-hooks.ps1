param(
    [string]$Port = "COM7",
    [ValidateSet("All", "Codex", "Claude", "OpenCode")]
    [string]$Target = "All",
    [string]$UserProfileRoot = $env:USERPROFILE,
    [switch]$SkipUserEnvironment
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $projectRoot "host\hooks"
$installRoot = Join-Path $UserProfileRoot ".ai-pet-hooks"
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"

New-Item -ItemType Directory -Force -Path $installRoot | Out-Null
Copy-Item -Path (Join-Path $sourceRoot "*") -Destination $installRoot `
    -Recurse -Force
if (-not $SkipUserEnvironment) {
    [Environment]::SetEnvironmentVariable("AI_PET_PORT", $Port, "User")
}
@{ port = $Port } | ConvertTo-Json |
    Set-Content -LiteralPath (Join-Path $installRoot "config.json") `
        -Encoding UTF8

function Add-CodexHook($Hooks, [string]$Name, [string]$Command) {
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
        if ($serialized -notmatch [regex]::Escape(".ai-pet-hooks")) {
            $Hooks.$Name = @($existing + $entry)
        }
    } else {
        $Hooks | Add-Member -NotePropertyName $Name `
            -NotePropertyValue @($entry)
    }
}

if ($Target -in @("All", "Codex")) {
    $codexHome = Join-Path $UserProfileRoot ".codex"
    New-Item -ItemType Directory -Force -Path $codexHome | Out-Null
    $codexHooksPath = Join-Path $codexHome "hooks.json"
    if (Test-Path -LiteralPath $codexHooksPath) {
        Copy-Item -LiteralPath $codexHooksPath `
            "$codexHooksPath.ai-pet-$timestamp.bak"
        $codexHooks = Get-Content -LiteralPath $codexHooksPath -Raw |
            ConvertFrom-Json
    } else {
        $codexHooks = [pscustomobject]@{
            hooks = [pscustomobject]@{}
        }
    }
    if (-not ($codexHooks.PSObject.Properties.Name -contains "hooks")) {
        $codexHooks | Add-Member -NotePropertyName hooks `
            -NotePropertyValue ([pscustomobject]@{})
    }
    $codexScript = Join-Path $installRoot "codex-hook.ps1"
    foreach ($event in @(
        "UserPromptSubmit", "PreToolUse", "PermissionRequest",
        "PostToolUse", "Stop"
    )) {
        $command = "powershell -NoProfile -ExecutionPolicy Bypass -File " +
            "`"$codexScript`" -Event $event"
        Add-CodexHook $codexHooks.hooks $event $command
    }
    $codexHooks | ConvertTo-Json -Depth 20 |
        Set-Content -LiteralPath $codexHooksPath -Encoding UTF8
    Write-Host "Codex lifecycle hooks installed."
}

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

$claudeSettings = Join-Path $UserProfileRoot ".claude\settings.json"
if ($Target -in @("All", "Claude") -and
    (Test-Path -LiteralPath $claudeSettings)) {
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

if ($Target -in @("All", "OpenCode") -and
    (Get-Command opencode -ErrorAction SilentlyContinue)) {
    $opencodePlugins = Join-Path $UserProfileRoot ".config\opencode\plugins"
    New-Item -ItemType Directory -Force -Path $opencodePlugins | Out-Null
    Copy-Item -LiteralPath (Join-Path $installRoot "opencode-plugin.js") `
        -Destination (Join-Path $opencodePlugins "ai-pet.js") -Force
    Write-Host "OpenCode plugin installed."
} elseif ($Target -in @("All", "OpenCode")) {
    Write-Host "OpenCode not detected; plugin template was not installed."
}

Write-Host "AI pet hooks installed at $installRoot"
Write-Host "Port: $Port"
