#include "CardRenderer.h"

#include <ArduinoJson.h>
#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>
#include <Logging.h>

#include "components/UITheme.h"
#include "fontIds.h"

CardRenderer::CardRenderer(GfxRenderer& renderer) : renderer_(renderer) {}

void CardRenderer::render(const std::string& cardJson) {
  JsonDocument doc;
  if (deserializeJson(doc, cardJson) != DeserializationError::Ok) {
    renderError("Invalid card JSON");
    return;
  }
  const std::string type = doc["type"] | "";
  if (type == "text" || type == "note") {
    const std::string title = doc["data"]["title"] | "";
    const std::string body = doc["data"]["body"] | "";
    renderText(title, body);
  } else if (type == "image") {
    const std::string path = doc["data"]["path"] | "";
    renderImageCard(path);
  } else if (type == "plugin") {
    const std::string title = doc["data"]["title"] | "";
    const std::string pluginName = doc["data"]["plugin_name"] | "";
    renderText(title.empty() ? pluginName : title, std::string("Plugin: ") + pluginName);
  } else {
    renderError("Unsupported card type");
  }
}

void CardRenderer::renderText(const std::string& title, const std::string& body) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer_.getScreenWidth();
  const auto pageHeight = renderer_.getScreenHeight();

  renderer_.clearScreen();

  if (!title.empty()) {
    GUI.drawHeader(renderer_, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight},
                   title.c_str());
  }

  const int lineHeight = renderer_.getLineHeight(UI_12_FONT_ID);
  int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;

  std::string remaining = body;
  while (!remaining.empty() && y + lineHeight <= pageHeight) {
    const auto pos = remaining.find('\n');
    std::string line;
    if (pos == std::string::npos) {
      line = remaining;
      remaining.clear();
    } else {
      line = remaining.substr(0, pos);
      remaining = remaining.substr(pos + 1);
    }
    renderer_.drawText(UI_12_FONT_ID, metrics.contentSidePadding, y, line.c_str());
    y += lineHeight;
  }

  renderer_.displayBuffer(HalDisplay::FAST_REFRESH);
}

void CardRenderer::renderError(const std::string& message) {
  const auto pageHeight = renderer_.getScreenHeight();
  const int lineHeight = renderer_.getLineHeight(UI_10_FONT_ID);
  const int y = (pageHeight - lineHeight) / 2;
  renderer_.clearScreen();
  renderer_.drawCenteredText(UI_10_FONT_ID, y, message.c_str());
  renderer_.displayBuffer(HalDisplay::FAST_REFRESH);
  LOG_ERR("CARD", "CardRenderer error: %s", message.c_str());
}

void CardRenderer::renderImageCard(const std::string& imagePath) {
  if (imagePath.empty() || !Storage.exists(imagePath.c_str())) {
    renderError("Image not found");
    return;
  }

  FsFile file;
  if (!Storage.openFileForRead("CardRenderer", imagePath, file)) {
    renderError("Cannot open image");
    return;
  }

  const auto pageWidth = renderer_.getScreenWidth();
  const auto pageHeight = renderer_.getScreenHeight();

  Bitmap bitmap(file, true);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    file.close();
    renderError("Invalid BMP file");
    return;
  }

  int x, y;
  if (bitmap.getWidth() > pageWidth || bitmap.getHeight() > pageHeight) {
    const float ratio =
        static_cast<float>(bitmap.getWidth()) / static_cast<float>(bitmap.getHeight());
    const float screenRatio = static_cast<float>(pageWidth) / static_cast<float>(pageHeight);
    if (ratio > screenRatio) {
      x = 0;
      y = static_cast<int>(
          (static_cast<float>(pageHeight) - static_cast<float>(pageWidth) / ratio) / 2.0f);
    } else {
      x = static_cast<int>(
          (static_cast<float>(pageWidth) - static_cast<float>(pageHeight) * ratio) / 2.0f);
      y = 0;
    }
  } else {
    x = (pageWidth - bitmap.getWidth()) / 2;
    y = (pageHeight - bitmap.getHeight()) / 2;
  }

  renderer_.clearScreen();
  renderer_.drawBitmap(bitmap, x, y, pageWidth, pageHeight, 0, 0);
  renderer_.displayBuffer(HalDisplay::FULL_REFRESH);
  file.close();
}
