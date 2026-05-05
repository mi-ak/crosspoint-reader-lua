#pragma once

#include "../Activity.h"
#include <string>

class LuaActivity final : public Activity {
public:
    LuaActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                const std::string& pluginName, std::function<void()> onGoBack);

    void onEnter() override;
    void onExit() override;
    void loop() override;
    void render(Activity::RenderLock&&) override {}  // unused: draw() called directly from loop()
    bool isLuaActivity() const override { return true; }
    std::string getResumableActivityName() const override { return pluginName; }

private:
    std::string pluginName;
    std::function<void()> onGoBack;
    bool scriptLoaded = false;
    bool inputReady   = false;   // true once all buttons released after launch
    bool fatalError   = false;
    std::string errorMessage;

    // Override render task to idle — Lua draw() runs on main loop for input sync
    [[noreturn]] void renderTaskLoop() override;

    void showError(const char* msg);
    void clearPluginResumeState();
};
