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
REGION_CONFIG_HEADER = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "region_config.h"
).read_text(encoding="utf-8")
REGION_CONFIG_SOURCE = (
    Path(__file__).parents[2] / "firmware" / "ai_pet" / "region_config.cpp"
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
    assert "constexpr uint16_t kSaveVersion = 10;" in SAVE_STORE_SOURCE
    assert "PetSaveDataV3" not in SAVE_STORE_SOURCE
    assert "PetSaveDataV2" not in SAVE_STORE_SOURCE
    assert "migrated" not in SAVE_STORE_SOURCE
    assert "fileSize != sizeof(PetSaveData)" in SAVE_STORE_SOURCE
    assert "constexpr uint16_t kSaveVersion = 10;" in GAME_STATE_SOURCE


def test_v1_2_save_data_has_techniques_and_16_bit_boss_hp():
    assert "uint8_t techniqueLevels[4];" in GAME_TYPES
    assert "uint16_t bossHp;" in GAME_TYPES
    assert "uint16_t bossMaxHp;" in GAME_TYPES
    assert "constexpr uint16_t kSaveVersion = 10;" in SAVE_STORE_SOURCE
    assert "constexpr uint16_t kSaveVersion = 10;" in GAME_STATE_SOURCE


def test_v1_2_region_config_has_bamboo_realm_and_difficulty_ladder():
    source = REGION_CONFIG_HEADER + REGION_CONFIG_SOURCE
    for token in (
        "base_boss_damage",
        "reward_bias[4]",
        "青竹灵境",
        "竹灵守卫",
        "灵竹玉佩",
        "60, 25, 6, 13, 15, 4",
        "90, 27, 7, 17, 17, 5",
        "220, 36, 10, 36, 25, 8",
    ):
        assert token in source


def test_qingyun_state_is_persisted_in_v1_1_save_data():
    for token in (
        "enum class AdventurePhase",
        "enum class AdventureEvent",
        "enum class EventResult",
        "enum class BattleResult",
        "uint8_t adventureProgress;",
        "uint8_t adventureEventMask;",
        "uint8_t adventureEventOrder;",
        "uint8_t bossUnlocked;",
        "AdventurePhase adventurePhase;",
        "AdventureEvent currentEvent;",
        "EventResult currentEventResult;",
        "uint8_t regionBossWins[5];",
        "uint8_t bossDefeated;",
        "uint8_t battleRound;",
        "uint8_t battleAttackTalisman;",
        "uint8_t battleGuardTalisman;",
        "uint16_t regionRound[5];",
        "uint8_t regionMisses[5];",
        "uint8_t regionTreasure[5];",
        "uint16_t staminaRecoverySeconds;",
        "uint16_t lastBossExperience;",
        "uint16_t lastBossCoins;",
        "uint8_t lastBossItems[4];",
        "uint8_t lastBossTreasure;",
    ):
        assert token in GAME_TYPES


def test_firmware_exposes_repeat_challenge_helpers():
    for signature in (
        "uint8_t attackDamage(uint32_t seed) const;",
        "uint8_t incomingDamage(uint32_t seed) const;",
        "uint16_t bossMaxHp() const;",
        "uint16_t completionExperience() const;",
        "uint16_t completionCoins() const;",
        "void resetRun();",
        "void grantItems(uint32_t seed);",
        "void rollTreasure(uint32_t seed);",
    ):
        assert signature in GAME_STATE_HEADER


def test_firmware_exposes_technique_api_and_effect_helpers():
    for signature in (
        "uint8_t techniqueLevel(uint8_t index) const;",
        "bool upgradeTechnique(uint8_t index);",
        "uint16_t recoveryIntervalSeconds() const;",
        "uint16_t maxEnergy(PetForm form, const uint8_t techniqueLevels[4])",
        "uint8_t bossEnergyRequirement() const;",
    ):
        assert signature in GAME_STATE_HEADER
    for token in (
        "kTechniqueThresholds",
        "kTechniqueCosts",
        "techniqueLevels",
        "TECHNIQUE_MAX_LEVEL",
    ):
        assert token in GAME_STATE_SOURCE + GAME_STATE_HEADER


def test_firmware_supports_region_token_direct_boss_prompt():
    for signature in (
        "bool canUseRegionTokenForBoss() const;",
        "bool useRegionTokenForBoss();",
    ):
        assert signature in GAME_STATE_HEADER
    for token in (
        "drawRegionTokenPrompt",
        "tokenPrompt_",
        "useRegionTokenForBoss()",
        "ItemType::RegionToken",
    ):
        assert token in UI_HEADER + UI_SOURCE + GAME_STATE_SOURCE


def test_ui_has_technique_overview_and_detail_pages():
    for token in (
        "UiPage::TechniqueDetail",
        "drawTechniqueOverview",
        "drawTechniqueDetail",
        "techniqueSelection_",
        "upgradeTechnique",
        "功法概览",
        "功法修炼",
    ):
        assert token in UI_HEADER + UI_SOURCE + GAME_TYPES


def test_v1_2_art_assets_are_wired_into_firmware_ui():
    for token in (
        '#include "assets/bamboo_realm_scene.h"',
        '#include "assets/bamboo_guardian.h"',
        '#include "assets/region_treasures.h"',
        "kBambooRealmScene",
        "kBambooGuardianPixels",
        "kBambooGuardianLargePixels",
        "kRegionTreasureQingyunSword",
        "kRegionTreasureSpiritBambooJade",
        "data.activeRegion == 1",
    ):
        assert token in UI_HEADER + UI_SOURCE


def test_firmware_inventory_rules_match_python_recovery_items():
    for item in (
        "SpiritHerb",
        "RecoveryPill",
        "AttackTalisman",
        "GuardTalisman",
        "RegionToken",
    ):
        assert item in GAME_TYPES
    assert "bool useItem(ItemType item);" in GAME_STATE_HEADER
    use_item_source = source_between(
        GAME_STATE_SOURCE,
        "bool GameState::useItem",
        "bool GameState::startAdventure",
    )
    assert "const uint8_t restore =" in use_item_source
    assert "data_.energy + restore" in use_item_source
    assert "data_.stamina + restore" in use_item_source
    assert "quantity--;" in use_item_source
    assert "return false;" in use_item_source


def test_inventory_has_item_and_treasure_pages():
    assert "void drawTreasureInventory(const PetSaveData& data);" in UI_HEADER
    assert "uint8_t inventoryTab_ = 0;" in UI_HEADER
    assert "inventoryTab_" in source_between(
        UI_SOURCE,
        "void GameUi::handle",
        "void GameUi::draw",
    )
    for label in ("宝物", "青云剑", "灵竹玉佩", "未得", "攻防", "闪避"):
        assert label in UI_SOURCE
    assert "data.regionTreasure[0]" in UI_SOURCE
    assert "data.regionTreasure[1]" in UI_SOURCE
    assert "kRegionTreasureQingyunSword" in UI_SOURCE
    assert "kRegionTreasureSpiritBambooJade" in UI_SOURCE


def test_qingyun_ui_shows_round_and_completion_rewards():
    for token in (
        "data.regionRound[data.activeRegion]",
        "data.lastBossExperience",
        "data.lastBossCoins",
        "data.lastBossItems",
        "data.lastBossTreasure",
    ):
        assert token in UI_SOURCE


def test_app_distinguishes_energy_and_stamina_recovery():
    runtime_source = source_between(
        APP_SOURCE,
        "void GameApp::update",
        "void GameApp::processInput",
    )
    assert "const uint8_t oldStamina" in runtime_source
    assert "state_.data().stamina > oldStamina" in runtime_source
    assert 'ui_.notify("体力恢复");' in runtime_source


def test_firmware_exposes_qingyun_adventure_and_event_api():
    for signature in (
        "bool startAdventure();",
        "void stopAdventure();",
        "AdventureTick tickAdventure(uint32_t seed);",
        "void acknowledgeAdventureResult();",
    ):
        assert signature in GAME_STATE_HEADER
    for value in ("25", "45", "65", "85"):
        assert value in GAME_STATE_SOURCE


def test_firmware_exposes_qingyun_auto_battle_api():
    for signature in (
        "bool startBossBattle(bool useAttackTalisman,",
        "BattleResult tickBossBattle(uint32_t seed);",
        "void retreatBoss();",
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
    assert "drawMenuFrame(state);" in UI_SOURCE
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
        "秘境际遇",
        "kRegions[data.activeRegion].name",
        "kRegions[data.activeRegion].boss_name",
        "仙宠状态",
        "已臻化境",
    ):
        assert label in UI_SOURCE


def test_battle_page_is_automatic_and_has_result_notice():
    for label in ("kRegions[data.activeRegion].boss_name", "自动交锋", "敌方气血",
                  "己方体力", "撤退"):
        assert label in UI_SOURCE
    for removed in ("K1 攻击", "K2 法诀", "K3 丹药", "K4 防御"):
        assert removed not in UI_SOURCE
    assert "startNotice(" in UI_SOURCE
    assert "drawNotice(" in UI_SOURCE


def test_game_app_ticks_adventure_and_battle_separately():
    assert "lastAdventureStepAt_" in APP_HEADER
    assert "lastBattleRoundAt_" in APP_HEADER
    assert "tickAdventure(" in APP_SOURCE
    assert "tickBossBattle(" in APP_SOURCE
    assert "tickExploration(" not in APP_SOURCE


def test_boss_ready_confirm_opens_battle_prompt_instead_of_restarting_adventure():
    adventure_input = source_between(
        UI_SOURCE,
        "} else if (page_ == UiPage::Adventure) {",
        "} else if (page_ == UiPage::Battle) {",
    )
    boss_ready_branch = source_between(
        adventure_input,
        "if (phase == AdventurePhase::BossReady)",
        "} else if (phase == AdventurePhase::Result) {",
    )
    assert "page_ = UiPage::Battle;" in boss_ready_branch
    assert "battlePrompt_ = true;" in boss_ready_branch
    assert "startQingyunAdventure()" not in boss_ready_branch


def test_qingyun_ui_renders_boss_with_sprite():
    for signature in (
        "void drawQingyunAdventure",
        "void drawQingyunEventResult",
        "void drawQingyunBossPrompt",
        "void drawQingyunScene",
    ):
        assert signature in UI_HEADER
    assert '#include "assets/qingyun_boss.h"' in UI_HEADER
    assert "kQingyunBossWidth" in UI_SOURCE
    assert "kQingyunBossFrameCount" in UI_SOURCE
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
    ):
        assert icon in UI_SOURCE


def test_qingyun_hud_uses_compact_icons_and_current_values_only():
    qingyun_source = source_between(
        UI_SOURCE,
        "void GameUi::drawQingyunAdventure",
        "void GameUi::drawQingyunEventResult",
    )
    assert "drawQingyunPet(" in UI_SOURCE
    assert "kHomeIconEnergy" in qingyun_source
    assert "kHomeIconHeart" in qingyun_source
    assert '"灵力"' not in qingyun_source
    assert '"体力"' not in qingyun_source
    assert '"/20"' not in qingyun_source
    assert '"/100"' not in qingyun_source


def test_qingyun_adventure_separates_large_pet_and_event_subjects():
    scene_source = source_between(
        UI_SOURCE,
        "void GameUi::drawQingyunScene",
        "void GameUi::drawQingyunAdventure",
    )
    event_source = source_between(
        UI_SOURCE,
        "void GameUi::drawQingyunEventSubject",
        "void GameUi::drawQingyunAdventure",
    )
    result_source = source_between(
        UI_SOURCE,
        "void GameUi::drawQingyunEventResult",
        "void GameUi::drawQingyunBossPrompt",
    )
    assert "constexpr uint8_t kQingyunAdventurePetSize = 48;" in UI_SOURCE
    assert "constexpr uint8_t kQingyunEventSubjectSize = 27;" in UI_SOURCE
    assert "drawQingyunPetLarge(data.form, 40, 42 + bob);" in scene_source
    assert "drawQingyunIcon(" not in scene_source
    assert "drawQingyunPet(" not in event_source
    assert "drawQingyunIconLarge(50, 50, kQingyunIconSpiritHerb);" in event_source
    assert "drawQingyunIconLarge(50, 50, kQingyunIconDemonBeast);" in event_source
    assert "drawQingyunEventSubject(data, data.currentEvent);" in result_source
    assert "drawQingyunScene(data, millis());" not in result_source


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
    assert "experienceForLevel(data.level)" in UI_SOURCE
    assert "cumulativeXpBeforeLevel(data.level)" in UI_SOURCE
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
    assert "page_ = UiPage::RegionSelect;" in home_source
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
    for icon in ("kHomeButtonMeritLog", "kHomeButtonInventory",
                 "kHomeButtonAdventure", "kHomeButtonStatus"):
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


def test_manual_ai_cultivation_exit_returns_home_without_result_page():
    assert "completeAiTask(uint32_t now, bool halved, bool acknowledge," in APP_HEADER
    assert "completeAiTask(now, true, true, false);" in APP_SOURCE
    assert "if (showResult) {" in APP_SOURCE
    assert "ui_.clearAiCultivation(now);" in APP_SOURCE


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
    end = APP_SOURCE.index("void GameApp::processSetCommand", start)
    preview_source = APP_SOURCE[start:end]
    assert "mutableData" not in preview_source
    assert "requestSave" not in preview_source


def test_status_reports_saved_and_preview_forms_separately():
    assert '"STATUS level=%u form=%u preview=%u preview_form=%u "' in APP_SOURCE
    assert "ui_.previewEnabled()" in APP_SOURCE
    assert "ui_.previewForm()" in APP_SOURCE
