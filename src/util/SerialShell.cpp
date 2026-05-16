#include "SerialShell.h"

#include <HalStorage.h>
#include "util/Ymodem.h"

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

String SerialShell::resolvePath(const String& path) {
    if (path.startsWith("/")) return path;
    if (cwd_ == "/") return "/" + path;
    return cwd_ + "/" + path;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SerialShell::begin() {
    cwd_ = "/";
    inputBuf_ = "";
    ymodeReceiving_ = false;
    active_ = true;
    Serial.print("x4> ");
}

void SerialShell::tick() {
    if (!active_) return;
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') {
            // ignore
        } else if (c == '\n') {
            Serial.println();
            handleCommand(inputBuf_);
            inputBuf_ = "";
            Serial.print("x4> ");
        } else if (c == 0x08 || c == (char)0x7f) {
            if (inputBuf_.length() > 0) {
                inputBuf_.remove(inputBuf_.length() - 1);
                Serial.print("\b \b");
            }
        } else {
            inputBuf_ += c;
            Serial.print(c);
        }
    }
}

// ---------------------------------------------------------------------------
// Command dispatcher
// ---------------------------------------------------------------------------

void SerialShell::handleCommand(const String& line) {
    String trimmed = line;
    trimmed.trim();
    if (trimmed.length() == 0) return;

    int spaceIdx = trimmed.indexOf(' ');
    String cmd, args;
    if (spaceIdx < 0) {
        cmd = trimmed;
        args = "";
    } else {
        cmd = trimmed.substring(0, spaceIdx);
        args = trimmed.substring(spaceIdx + 1);
        args.trim();
    }

    if (cmd == "ls") {
        cmdLs(args);
    } else if (cmd == "mv") {
        int sp = args.indexOf(' ');
        if (sp < 0) { Serial.println("Error: mv requires 2 arguments"); return; }
        String src = args.substring(0, sp);
        String dst = args.substring(sp + 1);
        dst.trim();
        cmdMv(src, dst);
    } else if (cmd == "rm") {
        if (args.length() == 0) { Serial.println("Error: rm requires a path"); return; }
        cmdRm(args);
    } else if (cmd == "mkdir") {
        if (args.length() == 0) { Serial.println("Error: mkdir requires a path"); return; }
        cmdMkdir(args);
    } else if (cmd == "cd") {
        if (args.length() == 0) { Serial.println("Error: cd requires a path"); return; }
        cmdCd(args);
    } else if (cmd == "cwd") {
        Serial.println(cwd_);
    } else if (cmd == "cat") {
        if (args.length() == 0) { Serial.println("Error: cat requires a path"); return; }
        cmdCat(args);
    } else if (cmd == "cp") {
        int sp = args.indexOf(' ');
        if (sp < 0) { Serial.println("Error: cp requires 2 arguments"); return; }
        String src = args.substring(0, sp);
        String dst = args.substring(sp + 1);
        dst.trim();
        cmdCp(src, dst);
    } else if (cmd == "help") {
        cmdHelp();
    } else if (cmd == "recv") {
        if (args.length() == 0) { Serial.println("Error: recv requires a path"); return; }
        Ymodem ymodem;
        ymodem.recvFile(resolvePath(args));
    } else if (cmd == "send") {
        if (args.length() == 0) { Serial.println("Error: send requires a path"); return; }
        Ymodem ymodem;
        ymodem.sendFile(resolvePath(args));
    } else {
        Serial.println("Error: unknown command: " + cmd);
    }
}

// ---------------------------------------------------------------------------
// Command implementations
// ---------------------------------------------------------------------------

void SerialShell::cmdLs(const String& arg) {
    String path = (arg.length() > 0) ? resolvePath(arg) : cwd_;
    FsFile dir = Storage.open(path.c_str(), O_RDONLY);
    if (!dir || !dir.isDir()) {
        Serial.println("Error: cannot open directory: " + path);
        return;
    }
    FsFile entry;
    while (entry.openNext(&dir, O_RDONLY)) {
        char name[256];
        entry.getName(name, sizeof(name));
        if (entry.isDir()) {
            Serial.println(String(name) + "\t[DIR]");
        } else {
            Serial.println(String(name) + "\t" + String((uint32_t)entry.fileSize()) + " bytes");
        }
        entry.close();
    }
    dir.close();
}

void SerialShell::cmdMv(const String& src, const String& dst) {
    if (Storage.rename(resolvePath(src).c_str(), resolvePath(dst).c_str())) {
        Serial.println("OK");
    } else {
        Serial.println("Error: rename failed");
    }
}

void SerialShell::cmdRm(const String& path) {
    if (Storage.remove(resolvePath(path).c_str())) {
        Serial.println("OK");
    } else {
        Serial.println("Error: remove failed");
    }
}

void SerialShell::cmdMkdir(const String& path) {
    if (Storage.mkdir(resolvePath(path).c_str(), true)) {
        Serial.println("OK");
    } else {
        Serial.println("Error: mkdir failed");
    }
}

void SerialShell::cmdCd(const String& path) {
    String newPath = resolvePath(path);
    if (Storage.exists(newPath.c_str())) {
        cwd_ = newPath;
    } else {
        Serial.println("Error: no such directory: " + newPath);
    }
}

void SerialShell::cmdCat(const String& path) {
    String content = Storage.readFile(resolvePath(path).c_str());
    Serial.print(content);
    if (!content.endsWith("\n")) Serial.println();
}

void SerialShell::cmdCp(const String& src, const String& dst) {
    FsFile srcFile, dstFile;
    if (!Storage.openFileForRead("SerialShell", resolvePath(src), srcFile)) {
        Serial.println("Error: cannot open source: " + src);
        return;
    }
    if (!Storage.openFileForWrite("SerialShell", resolvePath(dst), dstFile)) {
        srcFile.close();
        Serial.println("Error: cannot open destination: " + dst);
        return;
    }
    uint8_t buf[4096];
    size_t n;
    while ((n = srcFile.read(buf, sizeof(buf))) > 0) {
        dstFile.write(buf, n);
    }
    dstFile.sync();
    dstFile.close();
    srcFile.close();
    Serial.println("OK");
}

void SerialShell::cmdHelp() {
    Serial.println("Commands:");
    Serial.println("  ls [path]        list directory");
    Serial.println("  cd <path>        change directory");
    Serial.println("  cwd              print working directory");
    Serial.println("  cat <path>       print file contents");
    Serial.println("  cp <src> <dst>   copy file");
    Serial.println("  mv <src> <dst>   rename/move file");
    Serial.println("  rm <path>        remove file");
    Serial.println("  mkdir <path>     create directory");
    Serial.println("  recv <path>      receive file via YMODEM");
    Serial.println("  send <path>      send file via YMODEM");
    Serial.println("  help             show this help");
}
