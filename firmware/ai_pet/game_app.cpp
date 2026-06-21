#include "game_app.h"

#include "input_actions.h"

namespace {

constexpr uint32_t kSaveDelayMs = 1000;

}  // namespace

void GameApp::begin() {
  Serial.begin(board::kUsbBaud);
  const uint32_t waitStarted = millis();
  while (!Serial && millis() - waitStarted < 1200) {
    delay(10);
  }

  display_.begin();
  buttons_.begin();

  const bool fsReady = saves_.begin();
  const bool loaded = fsReady && saves_.load(state_);
  Serial.printf("GAME fs=%d save=%s\n", fsReady, loaded ? "loaded" : "new");
  if (fsReady && !loaded) {
    saves_.save(state_);
  }

  ui_.begin(display_);
  ui_.draw(state_, millis(), true);
  lastTickAt_ = millis();
  lastExplorationAt_ = millis();
}

void GameApp::update(uint32_t now) {
  processSerial();
  buttons_.update(now);
  processInput(now);

  if (now - lastTickAt_ >= 1000) {
    const uint32_t seconds = (now - lastTickAt_) / 1000;
    const uint16_t oldEnergy = state_.data().energy;
    state_.mutableData().playSeconds += seconds;
    if (state_.tickRuntime(seconds)) {
      requestSave();
      if (state_.data().energy > oldEnergy &&
          ui_.page() != UiPage::Battle) {
        ui_.notify("灵力恢复");
      }
      ui_.draw(state_, now, true);
    }
    lastTickAt_ += seconds * 1000;
  }

  if (now - lastExplorationAt_ >= 60000) {
    lastExplorationAt_ += 60000;
    const PetForm oldForm = state_.data().form;
    if (state_.tickExploration(state_.data().playSeconds)) {
      requestSave();
      if (state_.data().form != oldForm) {
        ui_.showEvolution(state_.data().form, now);
      } else {
        ui_.draw(state_, now, true);
      }
    }
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

void GameApp::processSerial() {
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
      } else if (serialCommand_.startsWith("{")) {
        AiEvent event{};
        const char* error = nullptr;
        if (AiEventProtocol::parse(serialCommand_, event, error)) {
          processAiEvent(event);
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

void GameApp::processAiEvent(const AiEvent& event) {
  if (event.kind == AiEventKind::Status) {
    ui_.showAiStatus(event.source, event.state, event.taskId, millis());
    printAck(event, "accepted");
    return;
  }

  if (!event.success) {
    ui_.showAiResult(event.source, false, 0, 0, false, millis());
    printAck(event, "accepted");
    return;
  }

  if (state_.hasProcessedTask(event.source, event.taskId)) {
    printAck(event, "duplicate");
    return;
  }

  const uint16_t oldExperience = state_.data().experience;
  const uint16_t oldCoins = state_.data().coins;
  const PetForm oldForm = state_.data().form;
  state_.applyAiTask(event.source, event.taskId, event.durationSeconds, true);
  const uint16_t experienceGain =
      state_.data().experience - oldExperience;
  const uint16_t coinGain = state_.data().coins - oldCoins;
  const bool evolved = state_.data().form != oldForm;
  ui_.showAiResult(event.source, true, experienceGain, coinGain, evolved,
                   millis());
  requestSave();
  printAck(event, "accepted", experienceGain, coinGain);
}

void GameApp::printAck(const AiEvent& event, const char* status,
                       uint16_t experience, uint16_t coins) {
  Serial.printf(
      "{\"type\":\"ack\",\"task_id\":\"%s\",\"status\":\"%s\","
      "\"experience\":%u,\"coins\":%u}\n",
      event.taskId, status, experience, coins);
}

void GameApp::printStatus() {
  const PetSaveData& data = state_.data();
  Serial.printf(
      "STATUS level=%u form=%u xp=%u mood=%u stamina=%u coins=%u "
      "energy=%u page=%u\n",
      data.level,
      static_cast<unsigned>(data.form),
      data.experience,
      data.mood,
      data.stamina,
      data.coins,
      data.energy,
      static_cast<unsigned>(ui_.page()));
}

void GameApp::requestSave() {
  savePending_ = true;
}
