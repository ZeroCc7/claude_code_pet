#include "game_app.h"

#include "input_actions.h"
#include "region_config.h"

namespace {

constexpr uint32_t kSaveDelayMs = 1000;
constexpr uint32_t kAdventureStepMs = 3000;
constexpr uint32_t kBattleRoundMs = 1200;
constexpr uint32_t kEventResultAutoAckMs = 5000;

}  // namespace

void GameApp::begin() {
  Serial.begin(board::kUsbBaud);
  const uint32_t waitStarted = millis();
  while (!Serial && millis() - waitStarted < 1200) {
    delay(10);
  }

  display_.begin();
  buttons_.begin();

  fsReady_ = saves_.begin();
  const bool loaded = fsReady_ && saves_.load(state_);
  Serial.printf("GAME fs=%d save=%s\n", fsReady_, loaded ? "loaded" : "new");
  if (fsReady_ && !loaded) {
    saves_.save(state_);
  }

  ui_.begin(display_);
  ui_.draw(state_, millis(), true);
  lastTickAt_ = millis();
  lastAdventureStepAt_ = millis();
  lastBattleRoundAt_ = millis();
}

void GameApp::update(uint32_t now) {
  processSerial(now);
  buttons_.update(now);
  processInput(now);

  if (ui_.consumeCultivationExit() && aiTaskActive_) {
    completeAiTask(now, true, true, false);
  }

  if (now - lastTickAt_ >= 1000) {
    const uint32_t seconds = (now - lastTickAt_) / 1000;
    const uint16_t oldEnergy = state_.data().energy;
    const uint8_t oldStamina = state_.data().stamina;
    state_.mutableData().playSeconds += seconds;
    if (state_.tickRuntime(seconds)) {
      requestSave();
      if (ui_.page() != UiPage::Battle) {
        if (state_.data().stamina > oldStamina) {
          ui_.notify("体力恢复");
        } else if (state_.data().energy > oldEnergy) {
          ui_.notify("灵力恢复");
        }
      }
      ui_.draw(state_, now, true);
    }
    lastTickAt_ += seconds * 1000;
  }

  if (state_.data().adventurePhase == AdventurePhase::Advancing &&
      now - lastAdventureStepAt_ >= kAdventureStepMs) {
    lastAdventureStepAt_ = now;
    const PetForm oldForm = state_.data().form;
    const AdventureTick result = state_.tickAdventure(now);
    if (result != AdventureTick::Inactive &&
        result != AdventureTick::WaitingForChoice) {
      requestSave();
      if (state_.data().form != oldForm) {
        ui_.showEvolution(state_.data().form, now);
      } else {
        if (result == AdventureTick::EventTriggered) {
          eventResultShownAt_ = now;
          ui_.notify("路有际遇");
        } else if (result == AdventureTick::EnergyDepleted) {
          ui_.notify("灵力耗尽");
        } else if (result == AdventureTick::BossUnlocked) {
          const char* boss = kRegions[state_.activeRegion()].boss_name;
          ui_.notify(boss);
        }
        ui_.draw(state_, now, true);
      }
    }
  }

  if (state_.data().inBattle &&
      now - lastBattleRoundAt_ >= kBattleRoundMs) {
    lastBattleRoundAt_ = now;
    const PetForm oldForm = state_.data().form;
    const BattleResult result = state_.tickBossBattle(now);
    requestSave();
    if (state_.data().form != oldForm) {
      ui_.showEvolution(state_.data().form, now);
    } else {
      if (result == BattleResult::Victory) {
        ui_.notify("首领已伏");
      } else if (result == BattleResult::Defeat) {
        ui_.notify("战败归山");
      } else if (result == BattleResult::EnergyDepleted) {
        ui_.notify("灵力耗尽");
      }
      ui_.draw(state_, now, true);
    }
  }

  if (state_.data().adventurePhase == AdventurePhase::Result &&
      eventResultShownAt_ != 0 &&
      now - eventResultShownAt_ >= kEventResultAutoAckMs) {
    state_.acknowledgeAdventureResult();
    eventResultShownAt_ = 0;
    requestSave();
    ui_.draw(state_, now, true);
  }

  if (aiTaskActive_ && now - aiTaskStartedAt_ >= 1800000) {
    completeAiTask(now, true, false);
  }

  if (savePending_ && now - lastSaveAt_ >= kSaveDelayMs) {
    if (saves_.save(state_)) {
      savePending_ = false;
      lastSaveAt_ = now;
      Serial.println("SAVE ok");
    } else {
      Serial.println("SAVE failed");
    }
  }

  ui_.draw(state_, now);
}

void GameApp::processInput(uint32_t now) {
  (void)now;
  for (size_t i = 0; i < board::kButtonCount; ++i) {
    const InputAction action = actionForButton(i, buttons_.state(i).event);
    if (action == InputAction::None) {
      continue;
    }
    const PetForm oldForm = state_.data().form;
    ui_.handle(action, state_);
    if (state_.data().form != oldForm) {
      ui_.showEvolution(state_.data().form, now);
    }
    requestSave();
  }
}

void GameApp::processSerial(uint32_t now) {
  while (Serial.available()) {
    const char next = static_cast<char>(Serial.read());
    if (next == '\r') {
      continue;
    }
    if (next == '\n') {
      serialCommand_.trim();
      if (serialOverflow_) {
        Serial.println("{\"type\":\"ack\",\"status\":\"error\",\"error\":\"too_long\"}");
      } else if (serialCommand_ == "STATUS") {
        printStatus();
      } else if (serialCommand_.startsWith("PREVIEW ")) {
        processPreviewCommand(serialCommand_, now);
      } else if (serialCommand_.startsWith("SET ")) {
        processSetCommand(serialCommand_);
      } else if (serialCommand_ == "RESET") {
        state_.reset();
        requestSave();
        Serial.println("RESET ok");
      } else if (serialCommand_.startsWith("{")) {
        AiEvent event{};
        const char* error = nullptr;
        if (AiEventProtocol::parse(serialCommand_, event, error)) {
          processAiEvent(event, now);
        } else {
          Serial.printf(
              "{\"type\":\"ack\",\"status\":\"error\",\"error\":\"%s\"}\n",
              error);
        }
      }
      serialCommand_ = "";
      serialOverflow_ = false;
    } else if (serialCommand_.length() <
               AiEventProtocol::kMaximumMessageBytes) {
      serialCommand_ += next;
    } else {
      serialOverflow_ = true;
    }
  }
}

void GameApp::processPreviewCommand(const String& command, uint32_t now) {
  if (command == "PREVIEW OFF") {
    ui_.clearPreviewForm();
    ui_.draw(state_, now, true);
    Serial.println("PREVIEW off");
    return;
  }

  const String value = command.substring(8);
  if (value.length() != 1 || value[0] < '0' || value[0] > '6') {
    Serial.println("PREVIEW error");
    return;
  }

  const uint8_t form = value[0] - '0';
  ui_.setPreviewForm(static_cast<PetForm>(form));
  ui_.draw(state_, now, true);
  Serial.printf("PREVIEW form=%u\n", form);
}

void GameApp::processSetCommand(const String& command) {
  const int space1 = command.indexOf(' ', 4);
  if (space1 < 0) {
    Serial.println("SET error: need field and value");
    return;
  }
  const String field = command.substring(4, space1);
  const long value = command.substring(space1 + 1).toInt();
  PetSaveData& d = state_.mutableData();

  if (field == "level") d.level = static_cast<uint8_t>(value);
  else if (field == "form") d.form = static_cast<PetForm>(value);
  else if (field == "xp") d.experience = static_cast<uint16_t>(value);
  else if (field == "energy") d.energy = static_cast<uint16_t>(value);
  else if (field == "stamina") d.stamina = static_cast<uint8_t>(value);
  else if (field == "mood") d.mood = static_cast<uint8_t>(value);
  else if (field == "coins") d.coins = static_cast<uint16_t>(value);
  else if (field == "t0") d.tendencies[0] = static_cast<uint16_t>(value);
  else if (field == "t1") d.tendencies[1] = static_cast<uint16_t>(value);
  else if (field == "t2") d.tendencies[2] = static_cast<uint16_t>(value);
  else if (field == "t3") d.tendencies[3] = static_cast<uint16_t>(value);
  else if (field == "progress") d.adventureProgress = static_cast<uint8_t>(value);
  else if (field == "boss") d.bossUnlocked = static_cast<uint8_t>(value);
  else if (field == "region") {
    const uint8_t r = static_cast<uint8_t>(value);
    if (r < kRegionCount) state_.selectRegion(r);
  }
  else if (field == "round") d.regionRound[d.activeRegion] = static_cast<uint16_t>(value);
  else if (field == "sword") d.regionTreasure[d.activeRegion] = static_cast<uint8_t>(value);
  else if (field == "unlock") {
    const uint8_t r = static_cast<uint8_t>(value);
    if (r < kRegionCount) d.regionsUnlocked |= (1U << r);
  }
  else if (field == "item0") d.inventory.items[0] = static_cast<uint16_t>(value);
  else if (field == "item1") d.inventory.items[1] = static_cast<uint16_t>(value);
  else if (field == "item2") d.inventory.items[2] = static_cast<uint16_t>(value);
  else if (field == "item3") d.inventory.items[3] = static_cast<uint16_t>(value);
  else if (field == "item4") d.inventory.items[4] = static_cast<uint16_t>(value);
  else if (field == "phase") d.adventurePhase = static_cast<AdventurePhase>(value);
  else {
    Serial.printf("SET error: unknown field '%s'\n", field.c_str());
    return;
  }
  requestSave();
  Serial.printf("SET %s=%ld ok\n", field.c_str(), value);
}

void GameApp::processAiEvent(const AiEvent& event, uint32_t now) {
  if (event.kind == AiEventKind::Start) {
    if (aiTaskActive_) {
      printAck("ignored");
      return;
    }
    aiTaskActive_ = true;
    strncpy(aiTaskSource_, event.source, sizeof(aiTaskSource_) - 1);
    aiTaskSource_[sizeof(aiTaskSource_) - 1] = '\0';
    aiTaskStartedAt_ = now;
    ui_.showAiActive(event.source, now);
    printAck("accepted");
    return;
  }

  if (event.kind != AiEventKind::End || !aiTaskActive_ ||
      strncmp(aiTaskSource_, event.source, sizeof(aiTaskSource_)) != 0) {
    printAck("ignored");
    return;
  }
  completeAiTask(now, false, true);
}

void GameApp::completeAiTask(uint32_t now, bool halved, bool acknowledge,
                             bool showResult) {
  const uint32_t elapsedSeconds = (now - aiTaskStartedAt_) / 1000;
  const uint32_t duration = constrain(elapsedSeconds, 60U, 1800U);
  const uint16_t oldExperience = state_.data().experience;
  const uint16_t oldCoins = state_.data().coins;
  const PetForm oldForm = state_.data().form;
  state_.completeAiTask(aiTaskSource_, duration, halved);
  const uint16_t experienceGain =
      state_.data().experience - oldExperience;
  const uint16_t coinGain = state_.data().coins - oldCoins;
  const bool evolved = state_.data().form != oldForm;
  if (showResult) {
    ui_.showAiResult(aiTaskSource_, experienceGain, coinGain, evolved, now);
  } else {
    ui_.clearAiCultivation(now);
  }
  aiTaskActive_ = false;
  aiTaskSource_[0] = '\0';
  aiTaskStartedAt_ = 0;
  requestSave();
  if (acknowledge) {
    printAck("accepted", experienceGain, coinGain);
  }
}

void GameApp::printAck(const char* status,
                       uint16_t experience, uint16_t coins) {
  Serial.printf(
      "{\"type\":\"ack\",\"status\":\"%s\","
      "\"experience\":%u,\"coins\":%u}\n",
      status, experience, coins);
}

void GameApp::printStatus() {
  const PetSaveData& data = state_.data();
  Serial.printf(
      "STATUS level=%u form=%u preview=%u preview_form=%u "
      "xp=%u mood=%u stamina=%u coins=%u "
      "energy=%u page=%u\n",
      data.level,
      static_cast<unsigned>(data.form),
      ui_.previewEnabled(),
      static_cast<unsigned>(ui_.previewEnabled()
                                ? ui_.previewForm()
                                : data.form),
      data.experience,
      data.mood,
      data.stamina,
      data.coins,
      data.energy,
      static_cast<unsigned>(ui_.page()));
}

void GameApp::requestSave() {
  if (!fsReady_) {
    return;
  }
  savePending_ = true;
}

#ifdef SIMULATOR_BUILD
void GameApp::sim_updateButtons(const bool states[4]) {
  // This is handled by button_scanner_sim.cpp via the global sim_button_states
  (void)states;
}

const uint16_t* GameApp::getDisplayFramebuffer() const {
  return const_cast<DisplayDevice&>(display_).raw().getFramebuffer();
}
#endif
