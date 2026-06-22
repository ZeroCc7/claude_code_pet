from pathlib import Path


UI_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_ui.cpp"
).read_text(encoding="utf-8")
UI_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_ui.h"
).read_text(encoding="utf-8")
APP_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_app.cpp"
).read_text(encoding="utf-8")
APP_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_app.h"
).read_text(encoding="utf-8")
PET_RENDERER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "pet_renderer.cpp"
).read_text(encoding="utf-8")
PET_RENDERER_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "pet_renderer.h"
).read_text(encoding="utf-8")


def function_source(source: str, signature: str) -> str:
    start = source.index(signature)
    body_start = source.index("{", start)
    depth = 0
    for index in range(body_start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise ValueError(f"unterminated function: {signature}")


def test_animation_frames_do_not_redraw_the_full_background():
    assert (
        "const bool fullRedraw = (dirty_ || force) && page_ == UiPage::Home;"
        in UI_SOURCE
    )
    assert """
  } else if (fullRedraw) {
    drawInkBackground(128);
    drawHomeHeader(state.data());
""" in UI_SOURCE


def test_dynamic_frames_do_not_clear_pet_region_before_drawing():
    assert "restoreHomeDynamicRegions();" not in UI_SOURCE


def test_pet_frame_is_composited_offscreen_before_one_display_write():
    assert "GFXcanvas16 petCanvas_" in UI_HEADER
    assert "petCanvas_.getBuffer()" in UI_SOURCE
    assert "tft.drawRGBBitmap(kPetRegionX, kPetRegionY" in UI_SOURCE
    assert "pet_.draw(petCanvas_" in UI_SOURCE


def test_final_forms_use_generated_body_sprite_arrays():
    for name in (
        "kPet_final_a1_frames",
        "kPet_final_a2_frames",
        "kPet_final_b1_frames",
        "kPet_final_b2_frames",
    ):
        assert name in PET_RENDERER


def test_final_form_effects_are_a_separate_renderer_layer():
    assert '#include "assets/pet_effects.h"' in PET_RENDERER
    assert "enum class PetEffect" in PET_RENDERER_HEADER
    assert "PetEffect effect" in PET_RENDERER_HEADER
    assert "drawEffect(" in PET_RENDERER
    assert "kPetEffectFrameCount" in PET_RENDERER


def test_success_feedback_ai_completion_and_evolution_start_pet_effects():
    feedback_source = function_source(UI_SOURCE, "void GameUi::startFeedback")
    ai_result_source = function_source(UI_SOURCE, "void GameUi::showAiResult")
    evolution_source = function_source(UI_SOURCE, "void GameUi::showEvolution")
    assert "startPetEffect(" in UI_HEADER
    assert "startPetEffect(PetEffect::Interaction" in feedback_source
    assert "startPetEffect(PetEffect::AiComplete" in ai_result_source
    assert "startPetEffect(PetEffect::Evolution" in evolution_source


def test_pet_effect_frames_keep_using_the_offscreen_pet_canvas():
    assert "pet_.draw(petCanvas_, form" in UI_SOURCE
    assert "petEffect_" in UI_SOURCE
    assert "tft.drawRGBBitmap(kPetRegionX, kPetRegionY" in UI_SOURCE


def test_operation_pages_are_composited_before_one_display_write():
    assert "GFXcanvas16 menuCanvas_" in UI_HEADER
    assert "drawMenuFrame(state.data());" in UI_SOURCE
    assert "tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer()" in UI_SOURCE


def test_selection_changes_use_menu_frame_instead_of_direct_full_redraw():
    assert (
        "const bool menuRedraw = (dirty_ || force) && page_ != UiPage::Home;"
        in UI_SOURCE
    )


def test_operation_pages_use_shared_ancient_ui_components():
    for name in (
        "drawTitlePlaque(",
        "drawPanel(",
        "drawFooterHints(",
        "drawProgressBar(",
    ):
        assert name in UI_SOURCE


def test_operation_pages_include_required_cultivation_information():
    for label in (
        "洞府培养",
        "体力 +20",
        "心境 +5",
        "秘境历练",
        "首领可战",
        "已镇守",
        "仙宠状态",
        "已臻化境",
    ):
        assert label in UI_SOURCE


def test_battle_page_has_four_action_tiles_and_result_notice():
    for label in ("攻击", "法诀", "丹药", "防御", "敌方气血", "己方体力"):
        assert label in UI_SOURCE
    assert "startNotice(" in UI_SOURCE
    assert "drawNotice(" in UI_SOURCE


def test_cloud_terrace_home_uses_larger_lower_pet_region():
    assert '#include "assets/cloud_terrace_home.h"' in UI_SOURCE
    assert "constexpr int16_t kPetRegionX = 28;" in UI_SOURCE
    assert "constexpr int16_t kPetRegionY = 14;" in UI_SOURCE
    assert "constexpr int16_t kPetRegionWidth = 72;" in UI_SOURCE
    assert "constexpr int16_t kPetRegionHeight = 100;" in UI_SOURCE
    assert "GFXcanvas16 petCanvas_{72, 100};" in UI_HEADER


def test_home_pet_is_lowered_fourteen_pixels_above_vitals():
    assert "int16_t petY = kPetRegionY + 33;" in UI_SOURCE
    assert "tft.fillRect(0, 114, 128, 14, kInkBlack);" in UI_SOURCE


def test_home_header_shows_level_current_xp_and_connection_icon():
    assert "drawHomeHeader(" in UI_SOURCE
    assert "data.experience % 20" in UI_SOURCE
    assert "aiState_ == AiWorkState::Idle" in UI_SOURCE
    assert "Offline/sleeping icon" in UI_SOURCE
    assert "Online/active icon" in UI_SOURCE
    assert 'tft.print("USB")' not in UI_SOURCE


def test_home_hud_uses_vital_icons_and_four_button_columns():
    assert "drawHomeVitals(" in UI_SOURCE
    for x in (32, 64, 96):
        assert f"tft.drawFastVLine({x}, 133, 24, 0x4A85);" in UI_SOURCE


def test_home_hud_uses_reference_art_icons_and_gold_panels():
    assert '#include "assets/home_ui_icons.h"' in UI_SOURCE
    assert "drawHomeIcon(" in UI_SOURCE
    assert "drawGoldPanel(" in UI_SOURCE
    for icon in ("kHomeIconLotus", "kHomeIconHeart",
                 "kHomeIconEnergy", "kHomeIconCrystal"):
        assert icon in UI_SOURCE


def test_home_button_row_uses_four_generated_art_icons():
    assert '#include "assets/home_button_icons.h"' in UI_SOURCE
    assert "drawButtonIcon(" in UI_SOURCE
    for icon in (
        "kHomeButtonInteract",
        "kHomeButtonCare",
        "kHomeButtonAdventure",
        "kHomeButtonStatus",
    ):
        assert icon in UI_SOURCE


def test_ai_work_uses_a_dedicated_cultivation_page():
    assert "UiPage::Cultivation" in UI_SOURCE
    assert "drawCultivation(" in UI_SOURCE
    assert "showAiStatus(" in UI_HEADER
    assert "showAiResult(" in UI_HEADER


def test_ai_event_uses_update_timestamp_to_avoid_immediate_timeout():
    assert "void processSerial(uint32_t now);" in APP_HEADER
    assert "void processAiEvent(const AiEvent& event, uint32_t now);" in APP_HEADER
    assert "processSerial(now);" in APP_SOURCE
    assert "processAiEvent(event, now);" in APP_SOURCE
    assert "showAiStatus(event.source, event.state, event.taskId, now);" in APP_SOURCE


def test_cultivation_page_supports_all_hook_states_and_timeout():
    for state in (
        "AiWorkState::Submitted",
        "AiWorkState::Thinking",
        "AiWorkState::Tool",
        "AiWorkState::Editing",
        "AiWorkState::Waiting",
        "AiWorkState::Blocked",
        "AiWorkState::Idle",
    ):
        assert state in UI_SOURCE
    assert "600000" in UI_SOURCE


def test_ai_result_can_show_rewards_and_evolution():
    assert "aiExperienceGain_" in UI_HEADER
    assert "aiCoinGain_" in UI_HEADER
    assert "aiEvolved_" in UI_HEADER


def test_local_progression_can_open_evolution_feedback():
    assert "showEvolution(" in UI_HEADER
    assert "ui_.showEvolution(" in (
        Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_app.cpp"
    ).read_text(encoding="utf-8")


def test_pet_form_preview_is_ui_only_and_shared_by_home_and_status():
    for method in (
        "setPreviewForm(PetForm form)",
        "clearPreviewForm()",
        "previewEnabled() const",
        "previewForm() const",
        "displayForm(PetForm savedForm) const",
    ):
        assert method in UI_HEADER
    assert "displayForm(data.form)" in UI_SOURCE
    assert "previewEnabled_" in UI_HEADER
    assert "previewForm_" in UI_HEADER


def test_serial_preview_command_supports_all_forms_and_off():
    assert "processPreviewCommand(" in APP_HEADER
    assert 'serialCommand_.startsWith("PREVIEW ")' in APP_SOURCE
    assert 'command == "PREVIEW OFF"' in APP_SOURCE
    assert 'Serial.println("PREVIEW error")' in APP_SOURCE
    assert "ui_.setPreviewForm(" in APP_SOURCE
    assert "ui_.clearPreviewForm();" in APP_SOURCE


def test_preview_command_does_not_mutate_or_save_game_state():
    start = APP_SOURCE.index("void GameApp::processPreviewCommand")
    end = APP_SOURCE.index("void GameApp::processAiEvent", start)
    preview_source = APP_SOURCE[start:end]
    assert "mutableData" not in preview_source
    assert "requestSave" not in preview_source


def test_status_reports_saved_and_preview_forms_separately():
    assert '"STATUS level=%u form=%u preview=%u preview_form=%u "' in APP_SOURCE
    assert "ui_.previewEnabled()" in APP_SOURCE
    assert "ui_.previewForm()" in APP_SOURCE
