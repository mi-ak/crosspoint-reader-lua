#pragma once
#include <string>

class GfxRenderer;

class CardRenderer {
 public:
  explicit CardRenderer(GfxRenderer& renderer);

  // Parse cardJson and render (type: text / note / image)
  void render(const std::string& cardJson);

  // Render a text card directly
  void renderText(const std::string& title, const std::string& body);

  // Render an error message
  void renderError(const std::string& message);

 private:
  GfxRenderer& renderer_;

  void renderImageCard(const std::string& imagePath);
};
