#pragma once

#include <Arduino.h>

class SerialShell {
public:
    void begin();
    void tick();

private:
    String cwd_;
    String inputBuf_;
    bool ymodeReceiving_ = false;
    bool active_ = false;

    String resolvePath(const String& path);
    void handleCommand(const String& line);

    void cmdLs(const String& arg);
    void cmdMv(const String& src, const String& dst);
    void cmdRm(const String& path);
    void cmdMkdir(const String& path);
    void cmdCd(const String& path);
    void cmdCat(const String& path);
    void cmdCp(const String& src, const String& dst);
    void cmdHelp();
};
