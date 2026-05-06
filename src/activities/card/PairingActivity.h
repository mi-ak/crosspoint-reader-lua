#pragma once

#include <functional>
#include <memory>
#include <string>

#include "CardRenderer.h"
#include "activities/ActivityWithSubactivity.h"

class CrossPointWebServer;

// Displays a QR code for X4 Card Bridge pairing.
// Automatically returns to caller when pairing is complete (session becomes ACTIVE)
// or when Back is pressed.
class PairingActivity final : public ActivityWithSubactivity {
 public:
  explicit PairingActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                           std::function<void()> goBack,
                           std::function<void(const std::string&)> goPlugin = nullptr);

  ~PairingActivity();
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(Activity::RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  enum class DisplayState { WAITING_PAIR, CONNECTED };
  DisplayState displayState_ = DisplayState::WAITING_PAIR;

  const std::function<void()> goBack_;
  std::function<void(const std::string&)> goPlugin_;
  std::string pairingUrl_;
  std::unique_ptr<CrossPointWebServer> webServer_;

  std::string currentCardJson_;
  bool cardNeedsRender_ = false;
  std::unique_ptr<CardRenderer> cardRenderer_;

  std::string pendingPluginName_;

  void onWifiSelectionComplete(bool connected);
};
