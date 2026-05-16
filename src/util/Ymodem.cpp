#include "Ymodem.h"

#include <HalStorage.h>

// ---------------------------------------------------------------------------
// Protocol constants
// ---------------------------------------------------------------------------
static const uint8_t YMODEM_SOH = 0x01;  // 128-byte block
static const uint8_t YMODEM_STX = 0x02;  // 1024-byte block
static const uint8_t YMODEM_EOT = 0x04;
static const uint8_t YMODEM_ACK = 0x06;
static const uint8_t YMODEM_NAK = 0x15;
static const uint8_t YMODEM_CAN = 0x18;
static const uint8_t YMODEM_CRC = 0x43;  // 'C'

static constexpr unsigned long YMODEM_TIMEOUT_MS = 10000UL;

// ---------------------------------------------------------------------------
// Internal helper: read one byte with timeout, returns -1 on timeout
// ---------------------------------------------------------------------------
static int ymodemReadByte(unsigned long timeoutMs) {
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (Serial.available()) return (uint8_t)Serial.read();
        yield();
    }
    return -1;
}

static void ymodemCancel() {
    Serial.write(YMODEM_CAN);
    Serial.write(YMODEM_CAN);
    Serial.flush();
}

// ---------------------------------------------------------------------------
// CRC-16/XMODEM  (poly=0x1021, init=0)
// ---------------------------------------------------------------------------
uint16_t Ymodem::calcCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return crc;
}

// ---------------------------------------------------------------------------
// recvFile
// ---------------------------------------------------------------------------
bool Ymodem::recvFile(const String& destPath) {
    // Request CRC mode
    Serial.write(YMODEM_CRC);
    Serial.flush();

    // ---- Block 0: file header ----
    int hdr = ymodemReadByte(YMODEM_TIMEOUT_MS);
    if (hdr < 0) {
        Serial.println("\r\nError: YMODEM timeout waiting for block 0");
        return false;
    }
    if (hdr != YMODEM_SOH && hdr != YMODEM_STX) {
        ymodemCancel();
        Serial.println("\r\nError: YMODEM unexpected header byte");
        return false;
    }

    size_t dataLen = (hdr == YMODEM_STX) ? 1024 : 128;

    int blk     = ymodemReadByte(1000);
    int blk_inv = ymodemReadByte(1000);
    if (blk < 0 || blk_inv < 0 || ((blk ^ blk_inv) != 0xFF) || blk != 0x00) {
        ymodemCancel();
        Serial.println("\r\nError: YMODEM bad block 0 header bytes");
        return false;
    }

    uint8_t buf0[1024];
    for (size_t i = 0; i < dataLen; i++) {
        int b = ymodemReadByte(1000);
        if (b < 0) { ymodemCancel(); Serial.println("\r\nError: YMODEM timeout in block 0 data"); return false; }
        buf0[i] = (uint8_t)b;
    }
    int ch = ymodemReadByte(1000), cl = ymodemReadByte(1000);
    if (ch < 0 || cl < 0) { ymodemCancel(); return false; }
    uint16_t recvCrc = ((uint16_t)ch << 8) | (uint8_t)cl;
    if (calcCrc16(buf0, dataLen) != recvCrc) {
        ymodemCancel();
        Serial.println("\r\nError: YMODEM CRC mismatch in block 0");
        return false;
    }

    // Parse filename (null-terminated at buf0[0])
    if (buf0[0] == '\0') {
        // End-of-batch indicator
        Serial.write(YMODEM_ACK);
        return true;
    }

    // ACK block 0 then send 'C' to request CRC for data blocks
    Serial.write(YMODEM_ACK);
    Serial.write(YMODEM_CRC);
    Serial.flush();

    // Open destination file
    FsFile dstFile;
    if (!Storage.openFileForWrite("Ymodem", destPath, dstFile)) {
        ymodemCancel();
        Serial.println("\r\nError: cannot create destination file: " + destPath);
        return false;
    }

    // ---- Data blocks ----
    uint8_t expectedBlk = 1;
    bool done = false;
    while (!done) {
        int h = ymodemReadByte(YMODEM_TIMEOUT_MS);
        if (h < 0) {
            dstFile.close();
            ymodemCancel();
            Serial.println("\r\nError: YMODEM timeout waiting for data block");
            return false;
        }

        if (h == YMODEM_EOT) {
            // First EOT: send NAK, expect second EOT
            Serial.write(YMODEM_NAK);
            Serial.flush();
            int h2 = ymodemReadByte(YMODEM_TIMEOUT_MS);
            if (h2 == YMODEM_EOT) {
                Serial.write(YMODEM_ACK);
                Serial.flush();
            }
            done = true;
            break;
        }

        if (h == YMODEM_CAN) {
            dstFile.close();
            Serial.println("\r\nError: YMODEM transfer cancelled by sender");
            return false;
        }

        if (h != YMODEM_SOH && h != YMODEM_STX) {
            continue;  // ignore unexpected bytes
        }

        size_t dLen = (h == YMODEM_STX) ? 1024 : 128;

        int bn     = ymodemReadByte(1000);
        int bn_inv = ymodemReadByte(1000);
        if (bn < 0 || bn_inv < 0 || ((bn ^ bn_inv) != 0xFF)) {
            ymodemCancel();
            dstFile.close();
            Serial.println("\r\nError: YMODEM bad block number");
            return false;
        }

        uint8_t pkt[1024];
        for (size_t i = 0; i < dLen; i++) {
            int b = ymodemReadByte(1000);
            if (b < 0) { ymodemCancel(); dstFile.close(); return false; }
            pkt[i] = (uint8_t)b;
        }

        int pc = ymodemReadByte(1000), pl = ymodemReadByte(1000);
        if (pc < 0 || pl < 0) { ymodemCancel(); dstFile.close(); return false; }
        uint16_t pktCrc = ((uint16_t)pc << 8) | (uint8_t)pl;

        if (calcCrc16(pkt, dLen) != pktCrc) {
            Serial.write(YMODEM_NAK);
            Serial.flush();
            continue;
        }

        if ((uint8_t)bn == expectedBlk) {
            dstFile.write(pkt, dLen);
            expectedBlk++;
        }
        // Duplicate block: just ACK without writing
        Serial.write(YMODEM_ACK);
        Serial.flush();
    }

    dstFile.sync();
    dstFile.close();

    // Send 'C' to signal ready for next file / end-of-batch
    Serial.write(YMODEM_CRC);
    Serial.flush();

    // Receive end-of-batch block 0 (empty filename) and ACK it
    int eh = ymodemReadByte(3000);
    if (eh == YMODEM_SOH || eh == YMODEM_STX) {
        size_t elen = (eh == YMODEM_STX) ? 1024 : 128;
        // Consume blk, ~blk, data, CRC (2 bytes)
        ymodemReadByte(1000); ymodemReadByte(1000);
        for (size_t i = 0; i < elen + 2; i++) ymodemReadByte(1000);
        Serial.write(YMODEM_ACK);
        Serial.flush();
    }

    Serial.println("\r\nYMODEM receive complete: " + destPath);
    return true;
}

// ---------------------------------------------------------------------------
// sendFile
// ---------------------------------------------------------------------------
bool Ymodem::sendFile(const String& srcPath) {
    FsFile srcFile;
    if (!Storage.openFileForRead("Ymodem", srcPath, srcFile)) {
        Serial.println("Error: cannot open file: " + srcPath);
        return false;
    }

    uint32_t fileSize = (uint32_t)srcFile.fileSize();

    // Extract filename from path
    String fname = srcPath;
    int lastSlash = fname.lastIndexOf('/');
    if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);

    // Wait for 'C' from receiver
    int c = ymodemReadByte(YMODEM_TIMEOUT_MS);
    if (c != YMODEM_CRC) {
        srcFile.close();
        Serial.println("Error: YMODEM no CRC request from receiver");
        return false;
    }

    // ---- Send block 0 (file header, 128 bytes, SOH) ----
    uint8_t hdrBuf[128];
    memset(hdrBuf, 0, sizeof(hdrBuf));
    size_t nameLen = (fname.length() < 64) ? fname.length() : 64;
    memcpy(hdrBuf, fname.c_str(), nameLen);
    String sizeStr = String(fileSize);
    memcpy(hdrBuf + nameLen + 1, sizeStr.c_str(), sizeStr.length());

    uint16_t hdrCrc = calcCrc16(hdrBuf, 128);
    Serial.write(YMODEM_SOH);
    Serial.write((uint8_t)0x00);
    Serial.write((uint8_t)0xFF);
    Serial.write(hdrBuf, 128);
    Serial.write((uint8_t)(hdrCrc >> 8));
    Serial.write((uint8_t)(hdrCrc & 0xFF));
    Serial.flush();

    // Wait for ACK + 'C'
    int a1 = ymodemReadByte(YMODEM_TIMEOUT_MS);
    if (a1 != YMODEM_ACK) {
        srcFile.close();
        Serial.println("\r\nError: YMODEM no ACK for block 0");
        return false;
    }
    int a2 = ymodemReadByte(YMODEM_TIMEOUT_MS);
    if (a2 != YMODEM_CRC) {
        srcFile.close();
        Serial.println("\r\nError: YMODEM no CRC request after block 0 ACK");
        return false;
    }

    // ---- Send data blocks (1024 bytes, STX) ----
    uint8_t dataBuf[1024];
    uint8_t blkNum = 1;
    while (true) {
        memset(dataBuf, 0x1A, sizeof(dataBuf));  // pad with SUB
        size_t n = srcFile.read(dataBuf, sizeof(dataBuf));
        if (n == 0) break;

        uint16_t dataCrc = calcCrc16(dataBuf, 1024);
        Serial.write(YMODEM_STX);
        Serial.write(blkNum);
        Serial.write((uint8_t)(~blkNum));
        Serial.write(dataBuf, 1024);
        Serial.write((uint8_t)(dataCrc >> 8));
        Serial.write((uint8_t)(dataCrc & 0xFF));
        Serial.flush();

        int ack = ymodemReadByte(YMODEM_TIMEOUT_MS);
        if (ack == YMODEM_NAK) {
            // Retransmit: seek back one block
            srcFile.seekCur(-(int32_t)n);
            continue;
        }
        if (ack != YMODEM_ACK) {
            srcFile.close();
            Serial.println("\r\nError: YMODEM no ACK for data block");
            return false;
        }
        blkNum++;
    }
    srcFile.close();

    // ---- EOT sequence ----
    Serial.write(YMODEM_EOT);
    Serial.flush();
    int nak = ymodemReadByte(YMODEM_TIMEOUT_MS);
    if (nak == YMODEM_NAK) {
        Serial.write(YMODEM_EOT);
        Serial.flush();
        int ack2 = ymodemReadByte(YMODEM_TIMEOUT_MS);
        if (ack2 != YMODEM_ACK) {
            Serial.println("\r\nError: YMODEM no ACK for second EOT");
            return false;
        }
    }

    // ---- End-of-batch block 0 (empty filename) ----
    int ec = ymodemReadByte(5000);
    if (ec == YMODEM_CRC) {
        uint8_t emptyBuf[128];
        memset(emptyBuf, 0, sizeof(emptyBuf));
        uint16_t emptyCrc = calcCrc16(emptyBuf, 128);
        Serial.write(YMODEM_SOH);
        Serial.write((uint8_t)0x00);
        Serial.write((uint8_t)0xFF);
        Serial.write(emptyBuf, 128);
        Serial.write((uint8_t)(emptyCrc >> 8));
        Serial.write((uint8_t)(emptyCrc & 0xFF));
        Serial.flush();
        ymodemReadByte(5000);  // final ACK
    }

    Serial.println("\r\nYMODEM send complete: " + srcPath);
    return true;
}
