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
GAME_TYPES = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_types.h"
).read_text(encoding="utf-8")
GAME_STATE_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_state.cpp"
).read_text(encoding="utf-8")
GAME_STATE_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "game_state.h"
).read_text(encoding="utf-8")
SAVE_STORE_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "save_store.cpp"
).read_text(encoding="utf-8")
AI_PROTOCOL_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "ai_event_protocol.h"
).read_text(encoding="utf-8")


def source_between(source: str, start_signature: str, end_signature: str) -> str:
    start = source.index(start_signature)
    end = source.index(end_signature, start)
    return source[start:end]


def test_v1_1_save_data_contains_inventory_and_merit_log():
    assert "struct InventoryData" in GAME_TYPES
    assert "uint16_t items[5];" in GAME_TYPES
    assert "struct AiTaskRecord" in GAME_TYPES
    assert "uint8_t source;" in GAME_TYPES
    assert "uint16_t durationSeconds;" in GAME_TYPES
    assert "uint16_t experienceReward;" in GAME_TYPES
    assert "uint16_t coinReward;" in GAME_TYPES
    assert "AiTaskRecord aiTaskRecords[10];" in GAME_TYPES
    assert "uint8_t aiTaskRecordIndex;" in GAME_TYPES
    assert "uint8_t aiTaskRecordCount;" in GAME_TYPES
    assert "recentTaskHashes" not in GAME_TYPES


def test_v1_1_save_store_rejects_legacy_layouts_instead_of_migrating():
    assert "constexpr uint16_t kSaveVersion = 6;" in SAVE_STORE_SOURCE
    assert "PetSaveDataV3" not in SAVE_STORE_SOURCE
    assert "PetSaveDataV2" not in SAVE_STORE_SOURCE
    assert "migrated" not in SAVE_STORE_SOURCE
    assert "fileSize != sizeof(PetSaveData)" in SAVE_STORE_SOURCE
    assert "constexpr uint16_t kSaveVersion = 6;" in GAME_STATE_SOURCE


def test_qingyun_state_is_persisted_in_v1_1_save_data():
    for token in (
        "enum class AdventurePhase",
        "enum class QingyunEvent",
        "enum class EventResult",
        "enum class BattleResult",
        "uint8_t qingyunProgress;",
        "uint8_t qingyunEventMask;",
        "uint8_t qingyunBossUnlocked;",
        "AdventurePhase adventurePhase;",
        "QingyunEvent currentEvent;",
        "EventResult currentEventResult;",
        "uint8_t qingyunBossWins;",
        "uint8_t qingyunBossDefeated;",
        "uint8_t battleRound;",
        "uint8_t battleAttackTalisman;",
        "uint8_t battleGuardTalisman;",
    ):
        assert token in GAME_TYPES


def test_firmware_inventory_rules_match_python_recovery_items():
    for item in (
        "SpiritHerb",
        "RecoveryPill",
        "AttackTalisman",
        "GuardTalisman",
        "QingyunToken",
    ):
        assert item in GAME_TYPES
    assert "bool useItem(ItemType item);" in GAME_STATE_HEADER
    use_item_source = source_between(
        GAME_STATE_SOURCE,
        "bool GameState::useItem",
        "bool GameState::startQingyunAdventure",
    )
    assert "data_.energy + 3" in use_item_source
    assert "data_.stamina + 20" in use_item_source
    assert "quantity--;" in use_item_source
    assert "return false;" in use_item_source


def test_firmware_exposes_qingyun_adventure_and_event_api():
    for signature in (
        "bool startQingyunAdventure();",
        "void stopQingyunAdventure();",
        "AdventureTick tickQingyunAdventure(uint32_t seed);",
        "EventResult resolveQingyunEvent(uint8_t choice, uint32_t seed);",
        "void acknowledgeAdventureResult();",
        "void abandonQingyunEvent();",
    ):
        assert signature in GAME_STATE_HEADER
    for value in ("25", "45", "65", "85"):
        assert value in GAME_STATE_SOURCE


def test_firmware_exposes_qingyun_auto_battle_api():
    for signature in (
        "bool startQingyunWolfBattle(bool useAttackTalisman,",
        "BattleResult tickQingyunWolfBattle(uint32_t seed);",
        "void retreatQingyunWolf();",
    ):
        assert signature in GAME_STATE_HEADER
    assert "battleAction(" not in GAME_STATE_HEADER


def test_animation_frames_do_not_redraw_the_full_background():
    assert (
        "const bool fullRedraw = (dirty_ || force) && page_ == UiPage::Home;"
        in UI_SOURCE
    )
    assert """
  } else if (fullRedraw) {
    drawHomeFrame(state.data(), now);
  } else if (page_ == UiPage::Home) {
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
    feedback_source = source_between(
        UI_SOURCE,
        "void GameUi::startFeedback",
        "void GameUi::drawFeedback",
    )
    ai_result_source = source_between(
        UI_SOURCE,
        "void GameUi::showAiResult",
        "void GameUi::showEvolution",
    )
    evolution_source = source_between(
        UI_SOURCE,
        "void GameUi::showEvolution",
        "void GameUi::setPreviewForm",
    )
    assert "startPetEffect(" in UI_HEADER
    assert """
  if (feedback == Feedback::MoodUp || feedback == Feedback::StaminaUp) {
    startPetEffect(PetEffect::Interaction, millis());
  }
""" in feedback_source
    assert "startPetEffect(evolved ? PetEffect::Evolution" in ai_result_source
    assert "startPetEffect(PetEffect::Evolution, now);" in evolution_source


def test_ai_pet_effects_start_when_cultivation_result_returns_home():
    draw_source = source_between(
        UI_SOURCE,
        "void GameUi::draw",
        "UiPage GameUi::page",
    )
    delayed_return_source = source_between(
        draw_source,
        "if (page_ == UiPage::Cultivation && aiResultActive_ &&",
        "if (petEffect_ != PetEffect::None",
    )
    evolution_source = source_between(
        UI_SOURCE,
        "void GameUi::showEvolution",
        "void GameUi::setPreviewForm",
    )

    assert "aiResultActive_ = false;" in delayed_return_source
    assert "startPetEffect(" not in delayed_return_source
    assert "aiResultActive_ = true;" in evolution_source
    assert "aiLastEventAt_ = now;" in evolution_source
    assert "page_ = UiPage::Cultivation;" in evolution_source


def test_pet_effect_frames_keep_using_the_offscreen_pet_canvas():
    assert "pet_.draw(petCanvas_, form" in UI_SOURCE
    assert "petEffect_" in UI_SOURCE
    assert "tft.drawRGBBitmap(kPetRegionX, kPetRegionY" in UI_SOURCE


def test_operation_pages_are_composited_before_one_display_write():
    assert "GFXcanvas16 menuCanvas_" in UI_HEADER
    assert "drawMenuFrame(state.data());" in UI_SOURCE
    assert "tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer()" in UI_SOURCE


def test_home_page_transitions_are_composited_before_one_display_write():
    assert "void drawHomeFrame(const PetSaveData& data, uint32_t now);" in UI_HEADER
    assert "drawHomeFrame(state.data(), now);" in UI_SOURCE
    home_frame_source = source_between(
        UI_SOURCE,
        "void GameUi::drawHomeFrame",
        "void GameUi::drawMenuFrame",
    )
    assert "renderTarget_ = &menuCanvas_;" in home_frame_source
    assert "drawInkBackground(128);" in home_frame_source
    assert "drawHomeHeader(data);" in home_frame_source
    assert "drawHome(data, now);" in home_frame_source
    assert "tft.drawRGBBitmap(0, 0, menuCanvas_.getBuffer(), 128, 160);" in home_frame_source


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
        "功德簿",
        "乾坤袋",
        "灵草",
        "回春丹",
        "青云山道",
        "山道抉择",
        "青云妖狼",
        "仙宠状态",
        "已臻化境",
    ):
        assert label in UI_SOURCE


def test_battle_page_is_automatic_and_has_result_notice():
    for label in ("青云妖狼", "自动交锋", "敌方气血", "己方体力", "K4撤退"):
        assert label in UI_SOURCE
    for removed in ("K1 攻击", "K2 法诀", "K3 丹药", "K4 防御"):
        assert removed not in UI_SOURCE
    assert "startNotice(" in UI_SOURCE
    assert "drawNotice(" in UI_SOURCE


def test_game_app_ticks_adventure_and_battle_separately():
    assert "lastAdventureStepAt_" in APP_HEADER
    assert "lastBattleRoundAt_" in APP_HEADER
    assert "tickQingyunAdventure(" in APP_SOURCE
    assert "tickQingyunWolfBattle(" in APP_SOURCE
    assert "tickExploration(" not in APP_SOURCE


def test_qingyun_ui_keeps_boss_as_geometric_placeholder():
    for signature in (
        "void drawQingyunAdventure",
        "void drawQingyunEvent",
        "void drawQingyunEventResult",
        "void drawQingyunBossPrompt",
        "void drawQingyunScene",
    ):
        assert signature in UI_HEADER
    assert "fillTriangle(" in UI_SOURCE
    assert "fillCircle(" in UI_SOURCE
    assert "qingyun_wolf" not in UI_SOURCE


def test_qingyun_scene_and_ui_icons_use_generated_assets():
    assert '#include "assets/qingyun_scene.h"' in UI_SOURCE
    assert '#include "assets/qingyun_ui_icons.h"' in UI_SOURCE
    assert "kQingyunScene" in UI_SOURCE
    for icon in (
        "kQingyunIconSpiritHerb",
        "kQingyunIconRecoveryPill",
        "kQingyunIconAttackTalisman",
        "kQingyunIconGuardTalisman",
        "kQingyunIconToken",
        "kQingyunIconDemonBeast",
        "kQingyunIconWoundedCultivator",
        "kQingyunIconShortcut",
        "kQingyunIconEnergy",
    ):
        assert icon in UI_SOURCE


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
    assert "aiActive_" in UI_SOURCE
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


def test_home_navigation_opens_merit_log_inventory_adventure_and_status():
    home_source = source_between(
        UI_SOURCE,
        "if (page_ == UiPage::Home)",
        "} else if (page_ == UiPage::MeritLog)",
    )
    assert "page_ = UiPage::MeritLog;" in home_source
    assert "page_ = UiPage::Inventory;" in home_source
    assert "page_ = UiPage::Adventure;" in home_source
    assert "page_ = UiPage::Status;" in home_source
    assert "state.interact()" not in home_source
    assert "UiPage::Care" not in UI_SOURCE
    assert "drawCare(" not in UI_SOURCE


def test_removed_care_actions_are_not_part_of_v1_1_state():
    assert "MeditationResult" not in GAME_TYPES
    assert "meditationCycleSeconds" not in GAME_TYPES
    assert "meditationsUsed" not in GAME_TYPES
    assert "void interact();" not in GAME_STATE_HEADER
    assert "bool feed();" not in GAME_STATE_HEADER
    assert "MeditationResult meditate();" not in GAME_STATE_HEADER


def test_home_button_row_uses_new_text_entries_and_existing_navigation_icons():
    assert '#include "assets/home_button_icons.h"' in UI_SOURCE
    assert "drawButtonIcon(" in UI_SOURCE
    assert 'text().draw(10, 153, "簿");' in UI_SOURCE
    assert 'text().draw(42, 153, "囊");' in UI_SOURCE
    for icon in ("kHomeButtonAdventure", "kHomeButtonStatus"):
        assert icon in UI_SOURCE


def test_inventory_and_merit_log_pages_have_expected_controls():
    assert "void GameUi::drawInventory" in UI_SOURCE
    assert "void GameUi::drawMeritLog" in UI_SOURCE
    assert "state.useItem(" in UI_SOURCE
    assert "meritPage_" in UI_HEADER
    assert "每页二则" in UI_SOURCE


def test_ai_work_uses_a_dedicated_cultivation_page():
    assert "UiPage::Cultivation" in UI_SOURCE
    assert "drawCultivation(" in UI_SOURCE
    assert "showAiActive(" in UI_HEADER
    assert "showAiResult(" in UI_HEADER


def test_simple_ai_protocol_and_single_active_task_controller():
    assert "AiEventKind::Start" in APP_SOURCE
    assert "AiEventKind::End" in APP_SOURCE
    assert "Start," in AI_PROTOCOL_HEADER
    assert "End," in AI_PROTOCOL_HEADER
    for removed in ("taskId", "AiWorkState", "durationSeconds", "success"):
        assert removed not in AI_PROTOCOL_HEADER
    assert "void processSerial(uint32_t now);" in APP_HEADER
    assert "void processAiEvent(const AiEvent& event, uint32_t now);" in APP_HEADER
    assert "processSerial(now);" in APP_SOURCE
    assert "processAiEvent(event, now);" in APP_SOURCE
    assert "aiTaskActive_" in APP_HEADER
    assert "aiTaskSource_" in APP_HEADER
    assert "aiTaskStartedAt_" in APP_HEADER
    assert "strncmp(aiTaskSource_, event.source" in APP_SOURCE
    assert "now - aiTaskStartedAt_ >= 1800000" in APP_SOURCE
    assert "completeAiTask(" in APP_SOURCE
    assert "completeAiTask(now, true, false);" in APP_SOURCE
    assert "completeAiTask(now, false, true);" in APP_SOURCE


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
