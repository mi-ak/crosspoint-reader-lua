#include "PairingActivity.h"

#include <GfxRenderer.h>
#include <WiFi.h>
#include <qrcode.h>

#include "CardBridgeSession.h"
#include "MappedInputManager.h"
#include "../../util/TimeService.h"
#include "../../util/LuaManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/CrossPointWebServer.h"

PairingActivity::PairingActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                 std::function<void()> goBack,
                                 std::function<void(const std::string&)> goPlugin)
    : ActivityWithSubactivity("PairingActivity", renderer, mappedInput),
      goBack_(std::move(goBack)),
      goPlugin_(std::move(goPlugin)) {}

PairingActivity::~PairingActivity() = default;

namespace {
constexpr int QR_PX = 5;           // Pixel size per QR module
constexpr int QR_VERSION = 4;      // Version 4 supports up to ~85 alphanumeric chars

static void drawQRCode(const GfxRenderer& renderer, int x, int y, const std::string& data) {
  QRCode qrcode;
  uint8_t qrcodeBytes[qrcode_getBufferSize(QR_VERSION)];
  qrcode_initText(&qrcode, qrcodeBytes, QR_VERSION, ECC_LOW, data.c_str());
  for (uint8_t cy = 0; cy < qrcode.size; cy++) {
    for (uint8_t cx = 0; cx < qrcode.size; cx++) {
      if (qrcode_getModule(&qrcode, cx, cy)) {
        renderer.fillRect(x + QR_PX * cx, y + QR_PX * cy, QR_PX, QR_PX, true);
      }
    }
  }
}
}  // namespace

void PairingActivity::onEnter() {
  ActivityWithSubactivity::onEnter();
  cardRenderer_ = std::make_unique<CardRenderer>(renderer);

  if (WiFi.status() != WL_CONNECTED) {
    enterNewActivity(new WifiSelectionActivity(renderer, mappedInput,
                                               [this](const bool connected) { onWifiSelectionComplete(connected); }));
    return;
  }

  // 6時間以上経過していれば NTP 再同期
  {
    const time_t now = time(nullptr);
    const time_t lastSync = TimeService::getInstance().getLastSuccessfulSyncEpoch();
    if (now - lastSync > 6 * 3600) {
      LOG_DBG("PAIR", "NTP sync: last=%ld now=%ld diff=%ld", (long)lastSync, (long)now, (long)(now - lastSync));
      TimeService::getInstance().syncNow();
    }
  }

  const std::string token = CardBridgeSession::getInstance().beginPairing();
  const String ip = WiFi.localIP().toString();
  pairingUrl_ = std::string("http://") + ip.c_str() + "/cards?code=" + token;

  if (!webServer_) {
    webServer_ = std::make_unique<CrossPointWebServer>();
    webServer_->begin();
    webServer_->setDisplayCardCallback([this](const std::string& cardJson) {
      currentCardJson_ = cardJson;
      cardNeedsRender_ = true;
      requestUpdate();
    });
    webServer_->setRunPluginCallback([this](const std::string& pluginName) {
      LOG_DBG("PAIR", "runPlugin queued: %s", pluginName.c_str());
      pendingPluginName_ = pluginName;
    });
  }
  requestUpdate();
}

void PairingActivity::onExit() {
  webServer_.reset();
  ActivityWithSubactivity::onExit();
}

void PairingActivity::onWifiSelectionComplete(const bool connected) {
  if (!connected) {
    exitActivity();
    goBack_();
    return;
  }
  exitActivity();

  const std::string token = CardBridgeSession::getInstance().beginPairing();
  const String ip = WiFi.localIP().toString();
  pairingUrl_ = std::string("http://") + ip.c_str() + "/cards?code=" + token;
  if (!webServer_) {
    webServer_ = std::make_unique<CrossPointWebServer>();
    webServer_->begin();
    webServer_->setDisplayCardCallback([this](const std::string& cardJson) {
      currentCardJson_ = cardJson;
      cardNeedsRender_ = true;
      requestUpdate();
    });
    webServer_->setRunPluginCallback([this](const std::string& pluginName) {
      LOG_DBG("PAIR", "runPlugin queued: %s", pluginName.c_str());
      pendingPluginName_ = pluginName;
    });
  }
  requestUpdate();
}

void PairingActivity::loop() {
  ActivityWithSubactivity::loop();

  if (subActivity) return;

  if (webServer_ && webServer_->isRunning()) {
    for (int i = 0; i < 10 && webServer_->isRunning(); i++) {
      webServer_->handleClient();
    }
  }

  if (!pendingPluginName_.empty()) {
    const std::string pluginName = pendingPluginName_;
    pendingPluginName_.clear();
    LOG_DBG("PAIR", "runPlugin: heap before webServer reset=%d", ESP.getFreeHeap());
    webServer_.reset();
    // Lua VM をリセット
    LuaManager::getInstance().end();
    // WiFi を切断してメモリを解放（プラグインに必要なヒープを確保）
    WiFi.disconnect(true);
    delay(100);
    LOG_DBG("PAIR", "runPlugin: heap after wifi disconnect=%d", ESP.getFreeHeap());
    if (goPlugin_) {
      goPlugin_(pluginName);
    }
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    webServer_.reset();
    goBack_();
    return;
  }

  // Tick session TTL
  CardBridgeSession::getInstance().tick(millis());

  // Stay on screen but show connected state when pairing is complete
  if (CardBridgeSession::getInstance().getState() == CardBridgeSession::State::ACTIVE) {
    if (displayState_ != DisplayState::CONNECTED) {
      displayState_ = DisplayState::CONNECTED;
      requestUpdate();
    }
    return;
  }

  // Return to caller if session expires while connected
  if (displayState_ == DisplayState::CONNECTED &&
      CardBridgeSession::getInstance().getState() == CardBridgeSession::State::EXPIRED) {
    webServer_.reset();
    goBack_();
    return;
  }
}

void PairingActivity::render(Activity::RenderLock&&) {
  if (cardNeedsRender_) {
    cardRenderer_->render(currentCardJson_);
    cardNeedsRender_ = false;
    return;
  }

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, "Card Bridge Pairing");

  if (displayState_ == DisplayState::WAITING_PAIR) {
    const int qrSize = QR_PX * 33;  // Version 4 QR is 33x33 modules
    const int qrX = (pageWidth - qrSize) / 2;
    const int qrY = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing * 2;

    if (!pairingUrl_.empty()) {
      drawQRCode(renderer, qrX, qrY, pairingUrl_);
    }

    const int textY = qrY + qrSize + metrics.verticalSpacing * 2;
    renderer.drawCenteredText(UI_12_FONT_ID, textY, "Scan to pair", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_12_FONT_ID, textY + renderer.getLineHeight(UI_12_FONT_ID) + 4,
                              pairingUrl_.c_str(), true);
  } else {
    const int centerY = (pageHeight - metrics.topPadding - metrics.headerHeight) / 2
                        + metrics.topPadding + metrics.headerHeight;
    renderer.drawCenteredText(UI_12_FONT_ID, centerY - renderer.getLineHeight(UI_12_FONT_ID),
                              "Connected", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_12_FONT_ID, centerY + metrics.verticalSpacing,
                              "Back: disconnect", true);
  }

  const auto labels = mappedInput.mapLabels(BaseTheme::HINT_BACK, "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
