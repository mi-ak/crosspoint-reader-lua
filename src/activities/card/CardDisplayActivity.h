#pragma once

#include <functional>
#include <string>

#include "../Activity.h"

// Displays a card (JSON) on the E-Ink screen.
// Supports text cards and BMP image cards.
// Exits via goBack callback when Back is pressed.
class CardDisplayActivity final : public Activity {
 public:
  explicit CardDisplayActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                               const std::string& cardJson,
                               const std::function<void()>& goBack);

  void onEnter() override;
  void loop() override;

 private:
  std::string cardJson_;
  std::function<void()> goBack_;
  bool rendered_ = false;
};
