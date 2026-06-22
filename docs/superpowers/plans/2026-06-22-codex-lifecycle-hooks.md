# Codex Lifecycle Hooks Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Install Codex lifecycle hooks that drive the RP2040 cultivation screen without replacing the existing Codex Desktop notification command.

**Architecture:** Extend the existing Codex adapter to translate official lifecycle event names into the project's serial protocol. Extend the installer with a Codex-only mode that copies hook scripts and merges idempotent entries into the user-level `hooks.json`.

**Tech Stack:** PowerShell, Python pytest, Codex `hooks.json`, USB serial

---

### Task 1: Codex lifecycle event adapter

**Files:**
- Modify: `host/hooks/codex-hook.ps1`
- Modify: `host/hooks/test_hook_payloads.py`

- [x] Add a failing source-structure test requiring lifecycle event names, tool-name extraction, and editing/tool mapping.
- [x] Run `py -3 -m pytest host/hooks/test_hook_payloads.py -q` and confirm the new test fails.
- [x] Update `codex-hook.ps1` so `UserPromptSubmit`, `PreToolUse`, `PermissionRequest`, `PostToolUse`, and `Stop` map to the agreed pet states.
- [x] Run the hook payload tests and PowerShell parser check.

### Task 2: Idempotent Codex-only installer

**Files:**
- Modify: `scripts/install-ai-hooks.ps1`
- Modify: `host/hooks/test_hook_payloads.py`

- [x] Add failing tests requiring a `Codex` install target, user-level `hooks.json`, preservation of `notify`, and idempotent pet-hook markers.
- [x] Run the focused tests and confirm failure.
- [x] Implement `-Target Codex`, merge the five lifecycle events into `~/.codex/hooks.json`, preserve unrelated hooks, and leave `config.toml` unchanged.
- [x] Run focused tests and PowerShell parser checks.

### Task 3: Install and verify

**Files:**
- Modify: `docs/ai-hooks-guide.md`
- External install target: `%USERPROFILE%\.ai-pet-hooks`
- External config target: `%USERPROFILE%\.codex\hooks.json`

- [x] Document the Codex lifecycle installation and `/hooks` trust step.
- [x] Run all host tests, `git diff --check`, and firmware compilation.
- [x] Run `scripts/install-ai-hooks.ps1 -Target Codex -Port COM5`.
- [x] Verify existing `notify` is unchanged and each pet Hook exists once.
- [x] Invoke installed lifecycle scripts manually and verify device ACK plus persistent cultivation page.
