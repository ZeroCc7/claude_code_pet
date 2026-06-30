# Qingyun Repeat Challenge Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add persistent Qingyun challenge rounds, capped difficulty scaling, randomized completion rewards, the one-time Qingyun Sword treasure, inventory treasure display, and passive stamina recovery.

**Architecture:** Keep Qingyun-specific rules in `GameState` and mirror them in the Python reference model. Extend `PetSaveData` directly because the new values are persistent, bump the incompatible save version, and keep UI state such as the inventory tab inside `GameUi`. Reuse the existing adventure and battle flow without creating a generic region or equipment engine.

**Tech Stack:** Arduino C++ for RP2040 firmware, Python dataclasses and pytest for the reference model, LittleFS A/B saves, Adafruit GFX UI.

---

### Task 1: Python reference rules

**Files:**
- Modify: `host/game_model/progression.py`
- Modify: `host/game_model/test_progression.py`

- [ ] **Step 1: Write failing tests for persistent progress and battle reset**

Add tests proving ordinary adventure exit preserves `qingyun_progress` and `qingyun_event_mask`, while defeat, retreat, and battle energy depletion reset both without incrementing `qingyun_round`.

- [ ] **Step 2: Run the focused tests and verify RED**

Run:

```powershell
py -3 -m pytest .\host\game_model\test_progression.py -q
```

Expected: failures because `qingyun_round` and reset behavior do not exist.

- [ ] **Step 3: Add the minimum persistent round state**

Add these dataclass fields:

```python
qingyun_round: int = 1
qingyun_misses: int = 0
has_qingyun_sword: bool = False
stamina_recovery_seconds: int = 0
```

Add a `_reset_qingyun_run()` helper that resets progress, event mask, event/result state, boss unlock state, and phase without changing the round.

- [ ] **Step 4: Write failing tests for scaling, rewards, treasure, and recovery**

Cover:

```python
assert state.qingyun_boss_max_hp() == expected
assert state.qingyun_completion_experience() in (7, 16, 18, 24)
assert reward_items contain one or two distinct consumable types
assert twentieth eligible clear grants the sword
assert sword modifies outgoing and incoming damage by 10 percent
assert state.tick_runtime(300) restores 5 stamina
```

- [ ] **Step 5: Implement deterministic helpers and seeded completion**

Implement capped difficulty for rounds 1–50, completion XP and coin formulas, seeded selection of one or two distinct consumables, sword chance `min(10, round)%` with a 20-clear pity, and stamina recovery. Use integer arithmetic and saturation.

- [ ] **Step 6: Run the focused tests and verify GREEN**

Run:

```powershell
py -3 -m pytest .\host\game_model\test_progression.py -q
```

Expected: all progression tests pass.

### Task 2: Firmware save layout and game rules

**Files:**
- Modify: `firmware/ai_pet/game_types.h`
- Modify: `firmware/ai_pet/game_state.h`
- Modify: `firmware/ai_pet/game_state.cpp`
- Modify: `firmware/ai_pet/save_store.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: Write failing source-contract tests**

Require save version 7 and persistent fields for round, pity count, sword ownership, stamina recovery time, and last completion rewards.

- [ ] **Step 2: Run source-contract tests and verify RED**

Run:

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
```

Expected: failures for absent save fields and version 7.

- [ ] **Step 3: Extend the save layout**

Add compact fixed-width fields to `PetSaveData`, including four last-reward item quantities for result display. Bump both `kSaveVersion` constants from 6 to 7. No migration is added.

- [ ] **Step 4: Mirror the tested Python rules in C++**

Add private helpers for capped round, scaled boss HP/damage, run reset, completion reward calculation, treasure roll, and saturating item/coin updates. Apply sword multipliers after existing talisman and form modifiers.

- [ ] **Step 5: Update battle outcomes**

Victory grants randomized rewards, rolls the sword, increments the round, and resets run progress. Defeat, retreat, and energy depletion reset run progress without incrementing the round. Ordinary adventure exit preserves progress.

- [ ] **Step 6: Add stamina recovery to `tickRuntime`**

Recover 5 stamina per accumulated 300 seconds, reset the timer at 100, and return `true` if either energy or stamina changed.

- [ ] **Step 7: Run model and source tests**

Run:

```powershell
py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q
```

Expected: all tests pass.

### Task 3: Inventory treasure page and round/reward UI

**Files:**
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: Write failing UI source tests**

Require an inventory tab field, `drawTreasureInventory`, Qingyun Sword labels/effects, round display, and completion reward display.

- [ ] **Step 2: Run UI tests and verify RED**

Run:

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
```

Expected: failures for missing treasure UI.

- [ ] **Step 3: Add two inventory tabs**

Use K2/K3 to switch between item and treasure tabs. Item selection and K1 item use only operate on the item tab. K4 continues to return home.

- [ ] **Step 4: Draw treasure and challenge status**

Show a locked silhouette and “尚未获得” before acquisition. After acquisition show “青云剑” and “伤害+10% / 减伤+10%”. Show the current round on adventure and boss screens and show completion XP, coins, item quantities, and treasure acquisition on the victory result.

- [ ] **Step 5: Run UI tests**

Run:

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
```

Expected: all UI source tests pass.

### Task 4: Application integration and verification

**Files:**
- Modify: `firmware/ai_pet/game_app.cpp`
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: Add a failing integration assertion**

Require runtime notifications to distinguish energy and stamina recovery and require battle outcome saves after reset/reward settlement.

- [ ] **Step 2: Implement minimal app notifications**

Capture old energy and stamina before `tickRuntime`; notify only outside battle, preferring the stamina message when stamina changed.

- [ ] **Step 3: Run the complete Python suite**

Run:

```powershell
py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q
```

Expected: all tests pass.

- [ ] **Step 4: Compile firmware**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: exit code 0 with Flash and RAM below board limits.

- [ ] **Step 5: Review the final diff**

Run:

```powershell
git diff --check
git status --short
```

Expected: only requested source, test, spec/plan files plus the user's pre-existing `.gitignore` modification are present.
