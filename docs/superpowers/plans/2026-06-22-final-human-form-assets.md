# Final Human Form Assets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate and integrate four animated human cultivation forms with layered idle aura and event-triggered skill effects.

**Architecture:** Generate one 2×2 idle body sheet and one 2×2 effect sheet per final form, then process them into transparent frames. Convert body and effect frames into separate RGB565/bitmask headers; `PetRenderer` composites the selected body and optional effect into the existing 72×100 off-screen canvas, while `GameUi` controls effect timing without changing save data.

**Tech Stack:** Built-in image generation, Python 3, Pillow, Arduino C++, Adafruit GFX/ST7735, pytest.

---

## File Structure

- Create `assets/raw/pets/final_a1/idle_2x2.png`: 太虚剑仙 raw body sheet.
- Create `assets/raw/pets/final_a2/idle_2x2.png`: 九转丹仙 raw body sheet.
- Create `assets/raw/pets/final_b1/idle_2x2.png`: 不灭武仙 raw body sheet.
- Create `assets/raw/pets/final_b2/idle_2x2.png`: 万灵仙尊 raw body sheet.
- Create `assets/raw/pets/final_*/effect_2x2.png`: one raw effect sheet per form.
- Create `assets/processed/pets/final_*/idle-1.png` through `idle-4.png`: transparent body frames.
- Create `assets/processed/pets/final_*/effect-1.png` through `effect-4.png`: transparent effect frames.
- Modify `scripts/convert_pet_sprites.py`: convert final body frames into a separate final-form sprite section.
- Create `scripts/convert_pet_effects.py`: convert final effect frames into RGB565 and masks.
- Modify `firmware/ai_pet/assets/pet_sprites.h`: generated body data.
- Create `firmware/ai_pet/assets/pet_effects.h`: generated effect data.
- Modify `firmware/ai_pet/pet_renderer.h`: expose effect state and layered draw API.
- Modify `firmware/ai_pet/pet_renderer.cpp`: select final body arrays and composite effect frames.
- Modify `firmware/ai_pet/game_ui.h`: track the current pet effect and start time.
- Modify `firmware/ai_pet/game_ui.cpp`: start effects for interaction, AI completion, and evolution and keep off-screen compositing.
- Modify `host/game_model/test_ui_refresh.py`: assert final sprite selection, layer separation, triggers, and redraw behavior.

### Task 1: Add failing integration assertions

**Files:**
- Modify: `host/game_model/test_ui_refresh.py`

- [ ] **Step 1: Add tests for final body assets and layered effects**

```python
PET_RENDERER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "pet_renderer.cpp"
).read_text(encoding="utf-8")
PET_RENDERER_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "pet_renderer.h"
).read_text(encoding="utf-8")


def test_final_forms_use_generated_body_sprite_arrays():
    assert '#include "assets/pet_effects.h"' in PET_RENDERER
    for name in (
        "kPet_final_a1_frames",
        "kPet_final_a2_frames",
        "kPet_final_b1_frames",
        "kPet_final_b2_frames",
    ):
        assert name in PET_RENDERER


def test_final_form_effects_are_a_separate_renderer_layer():
    assert "enum class PetEffect" in PET_RENDERER_HEADER
    assert "PetEffect effect" in PET_RENDERER_HEADER
    assert "drawEffect(" in PET_RENDERER
    assert "kPetEffectFrameCount" in PET_RENDERER


def test_success_feedback_ai_completion_and_evolution_start_pet_effects():
    assert "startPetEffect(" in UI_HEADER
    assert "startPetEffect(PetEffect::Interaction" in UI_SOURCE
    assert "startPetEffect(PetEffect::AiComplete" in UI_SOURCE
    assert "startPetEffect(PetEffect::Evolution" in UI_SOURCE


def test_pet_effect_frames_keep_using_the_offscreen_pet_canvas():
    assert "pet_.draw(petCanvas_, form" in UI_SOURCE
    assert "petEffect_" in UI_SOURCE
    assert "tft.drawRGBBitmap(kPetRegionX, kPetRegionY" in UI_SOURCE
```

- [ ] **Step 2: Run the tests and verify failure**

Run:

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
```

Expected: the four new tests fail because final sprite arrays and `PetEffect` do not exist.

- [ ] **Step 3: Commit the tests**

```powershell
git add -- host/game_model/test_ui_refresh.py
git commit -m "test: define final human sprite integration"
```

### Task 2: Generate four human idle sheets

**Files:**
- Create: `assets/raw/pets/final_a1/idle_2x2.png`
- Create: `assets/raw/pets/final_a2/idle_2x2.png`
- Create: `assets/raw/pets/final_b1/idle_2x2.png`
- Create: `assets/raw/pets/final_b2/idle_2x2.png`

- [ ] **Step 1: Generate 太虚剑仙·凌霄**

Use built-in image generation with a strict 2×2 pixel-art sheet: handsome young male sword immortal, cyan-white robes, dark-jade qilin horns, restrained cyan sword aura, full body centered in each cell, stable feet line, four idle phases, solid `#FF00FF` background, no grid lines, no text, no detached wide sword slash.

- [ ] **Step 2: Generate 九转丹仙·瑶华**

Use the same sheet geometry and pixel scale: elegant young female alchemy immortal, ivory and cinnabar robes, dark-jade qilin horns, gold-red furnace and lotus motifs, restrained attached ember aura, no detached large flame.

- [ ] **Step 3: Generate 不灭武仙·镇岳**

Use the same sheet geometry and pixel scale: powerful handsome young male martial immortal, black-gold fitted battle robes, dark-jade qilin horns, gold thunder markings, compact fists and no wide punch effect.

- [ ] **Step 4: Generate 万灵仙尊·清岚**

Use the same sheet geometry and pixel scale: elegant young female spirit immortal, cyan-jade layered robes, dark-jade qilin horns, spirit bells and compact circular glyphs, no detached summons.

- [ ] **Step 5: Inspect all four raw sheets**

Verify each sheet has exactly four complete figures, consistent identity and scale, no body part crossing a cell edge, and visible magenta separation.

### Task 3: Generate four independent effect sheets

**Files:**
- Create: `assets/raw/pets/final_a1/effect_2x2.png`
- Create: `assets/raw/pets/final_a2/effect_2x2.png`
- Create: `assets/raw/pets/final_b1/effect_2x2.png`
- Create: `assets/raw/pets/final_b2/effect_2x2.png`

- [ ] **Step 1: Generate four-frame cyan sword-domain effect**

Generate an effect-only 2×2 sheet with cyan sword light, small orbiting blade glyphs, and a short vertical burst. Keep the center readable for the body layer and use solid `#FF00FF`.

- [ ] **Step 2: Generate four-frame alchemy lotus effect**

Generate an effect-only 2×2 sheet with gold-red lotus flame, furnace sigils, and rising embers. Keep the center transparent for the body.

- [ ] **Step 3: Generate four-frame golden thunder-fist effect**

Generate an effect-only 2×2 sheet with compact gold lightning, ground runes, and a short fist-halo pulse. Keep the center transparent.

- [ ] **Step 4: Generate four-frame spirit-ring effect**

Generate an effect-only 2×2 sheet with cyan-jade spirit rings, small wisps, and rotating runes. Keep the center transparent.

- [ ] **Step 5: Inspect all four raw effect sheets**

Verify the effect occupies the same central 64×82 target region, does not touch cell borders, and remains readable when placed behind and around a human sprite.

### Task 4: Process generated sheets into transparent frames

**Files:**
- Create: `assets/processed/pets/final_*/idle-1.png` through `idle-4.png`
- Create: `assets/processed/pets/final_*/effect-1.png` through `effect-4.png`

- [ ] **Step 1: Process each body sheet**

Run the installed `generate2dsprite.py process` command for a 2-row, 2-column sheet with shared scale, feet alignment, largest-component body selection, and magenta cleanup.

- [ ] **Step 2: Process each effect sheet**

Run the same processor with all-component selection, center alignment, shared scale, and magenta cleanup.

- [ ] **Step 3: Inspect frame PNGs and GIF previews**

Reject and regenerate any body with unstable height, face, horns, costume, or feet position. Reject effects with residual magenta, clipped particles, or a filled center that hides the person.

- [ ] **Step 4: Commit accepted source and processed art**

```powershell
git add -- assets/raw/pets/final_a1 assets/raw/pets/final_a2 assets/raw/pets/final_b1 assets/raw/pets/final_b2 assets/processed/pets/final_a1 assets/processed/pets/final_a2 assets/processed/pets/final_b1 assets/processed/pets/final_b2
git commit -m "art: add final human form animations"
```

### Task 5: Convert body and effect frames for firmware

**Files:**
- Modify: `scripts/convert_pet_sprites.py`
- Create: `scripts/convert_pet_effects.py`
- Modify: `firmware/ai_pet/assets/pet_sprites.h`
- Create: `firmware/ai_pet/assets/pet_effects.h`

- [ ] **Step 1: Extend body conversion inputs**

Add these entries to `FORMS`:

```python
"final_a1": ROOT / "assets/processed/pets/final_a1",
"final_a2": ROOT / "assets/processed/pets/final_a2",
"final_b1": ROOT / "assets/processed/pets/final_b1",
"final_b2": ROOT / "assets/processed/pets/final_b2",
```

Use a per-form frame-size mapping so existing forms remain 62×62 while final forms use a maximum 64×82 canvas and feet alignment.

- [ ] **Step 2: Add the effect converter**

Create `scripts/convert_pet_effects.py` with four form directories, four frames per form, RGB565 pixel arrays, alpha masks at threshold 96, and generated declarations:

```cpp
constexpr uint8_t kPetEffectWidth = 64;
constexpr uint8_t kPetEffectHeight = 82;
constexpr uint8_t kPetEffectFrameCount = 4;
const PetEffectFrame kPetEffect_final_a1_frames[];
const PetEffectFrame kPetEffect_final_a2_frames[];
const PetEffectFrame kPetEffect_final_b1_frames[];
const PetEffectFrame kPetEffect_final_b2_frames[];
```

- [ ] **Step 3: Run both converters**

```powershell
py -3 .\scripts\convert_pet_sprites.py
py -3 .\scripts\convert_pet_effects.py
```

Expected: both commands exit 0 and write firmware headers plus firmware preview PNGs.

- [ ] **Step 4: Commit converters and generated headers**

```powershell
git add -- scripts/convert_pet_sprites.py scripts/convert_pet_effects.py firmware/ai_pet/assets/pet_sprites.h firmware/ai_pet/assets/pet_effects.h
git commit -m "feat: convert final human sprites for firmware"
```

### Task 6: Render final bodies and layered effects

**Files:**
- Modify: `firmware/ai_pet/pet_renderer.h`
- Modify: `firmware/ai_pet/pet_renderer.cpp`

- [ ] **Step 1: Define the renderer effect API**

Add:

```cpp
enum class PetEffect : uint8_t {
  None,
  Interaction,
  AiComplete,
  Evolution,
};

void draw(Adafruit_GFX& target, PetForm form, int16_t x, int16_t y,
          uint32_t now, PetEffect effect = PetEffect::None,
          uint32_t effectElapsed = 0);
```

- [ ] **Step 2: Select final body arrays**

Map `FinalA1`, `FinalA2`, `FinalB1`, and `FinalB2` to their generated body arrays and remove the procedural final-human fallback.

- [ ] **Step 3: Draw the selected effect layer**

Choose the effect array by `PetForm`, select `(effectElapsed / 100) % 4`, and draw it before the body with `drawRGBBitmap`. `PetEffect::None` skips the full effect; normal body frames retain only their generated weak aura.

- [ ] **Step 4: Run targeted tests**

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
```

Expected: body and renderer-layer tests pass; trigger tests may still fail.

- [ ] **Step 5: Commit renderer integration**

```powershell
git add -- firmware/ai_pet/pet_renderer.h firmware/ai_pet/pet_renderer.cpp
git commit -m "feat: render final human forms and effects"
```

### Task 7: Trigger effects from interaction, AI completion, and evolution

**Files:**
- Modify: `firmware/ai_pet/game_ui.h`
- Modify: `firmware/ai_pet/game_ui.cpp`

- [ ] **Step 1: Add effect state**

Add:

```cpp
void startPetEffect(PetEffect effect, uint32_t now);
PetEffect petEffect_ = PetEffect::None;
uint32_t petEffectStartedAt_ = 0;
```

- [ ] **Step 2: Trigger interaction effects**

When `Feedback::MoodUp` or `Feedback::StaminaUp` starts, call:

```cpp
startPetEffect(PetEffect::Interaction, millis());
```

- [ ] **Step 3: Trigger AI completion and evolution effects**

In successful `showAiResult`, call:

```cpp
startPetEffect(evolved ? PetEffect::Evolution : PetEffect::AiComplete, now);
```

In `showEvolution`, call:

```cpp
startPetEffect(PetEffect::Evolution, now);
```

- [ ] **Step 4: Pass active effects to the renderer**

Keep an effect active for 500 ms, redraw the home pet region every 100 ms while active, and call:

```cpp
pet_.draw(petCanvas_, form, petX, petY - kPetRegionY, now,
          petEffect_, now - petEffectStartedAt_);
```

Clear `petEffect_` after the duration and restore normal 400 ms idle cadence.

- [ ] **Step 5: Run targeted tests**

```powershell
py -3 -m pytest .\host\game_model\test_ui_refresh.py -q
```

Expected: all tests pass.

- [ ] **Step 6: Commit event wiring**

```powershell
git add -- firmware/ai_pet/game_ui.h firmware/ai_pet/game_ui.cpp host/game_model/test_ui_refresh.py
git commit -m "feat: trigger final form skill effects"
```

### Task 8: Full verification and hardware preview

**Files:**
- No new files.

- [ ] **Step 1: Run all host tests**

```powershell
py -3 -m pytest .\host\game_model .\host\hooks .\host\diagnostics -q
```

Expected: all tests pass.

- [ ] **Step 2: Compile firmware**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\compile-firmware.ps1
```

Expected: compilation succeeds for `rp2040:rp2040:waveshare_rp2040_zero:flash=2097152_262144`; Flash and RAM remain below limits.

- [ ] **Step 3: Upload to COM7**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\upload-firmware.ps1 -Port COM7
```

Expected: upload succeeds.

- [ ] **Step 4: Preview all final forms**

At 115200 baud send, one line at a time:

```text
PREVIEW 3
PREVIEW 4
PREVIEW 5
PREVIEW 6
PREVIEW OFF
```

Expected: all four generated human forms appear above the vitals panel; `PREVIEW OFF` restores the saved form.

- [ ] **Step 5: Test event effects**

Trigger one successful K1 interaction and one isolated AI hook completion session. Confirm the full effect plays without flashing the background and returns to idle.

- [ ] **Step 6: Review the final diff**

```powershell
git status --short
git diff --check
```

Expected: no accidental `build/`, `tools/`, cache, or unrelated hardware files are staged.

