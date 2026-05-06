#include "CardDisplayActivity.h"

#include <GfxRenderer.h>

#include "CardRenderer.h"
#include "MappedInputManager.h"

CardDisplayActivity::CardDisplayActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                         const std::string& cardJson,
                                         const std::function<void()>& goBack)
    : Activity("CardDisplay", renderer, mappedInput), cardJson_(cardJson), goBack_(goBack) {}

void CardDisplayActivity::onEnter() {
  Activity::onEnter();
  rendered_ = false;
}

void CardDisplayActivity::loop() {
  if (!rendered_) {
    CardRenderer cr(renderer);
    cr.render(cardJson_);
    rendered_ = true;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    goBack_();
    return;
  }
}
