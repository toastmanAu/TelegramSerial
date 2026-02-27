/*
 * TelegramSerialTest.ino — CYD (ESP32-2432S028R) test sketch
 *
 * Tests:
 *   1. WiFi connect
 *   2. Single message send
 *   3. println() / printf() interface
 *   4. Queue drain (burst of messages)
 *   5. Mirror to Serial verified locally
 *   6. Periodic heartbeat every 30s
 *
 * Fill in credentials below, flash, then watch both Serial monitor
 * and your Telegram chat for output.
 */

#include <Arduino.h>
#include "TelegramSerial.h"

// ── Credentials — fill these in ──────────────────────────────────────────────
#define WIFI_SSID   "YourNetwork"
#define WIFI_PASS   "YourPassword"
#define BOT_TOKEN   "123456789:ABC-YourBotTokenHere"
#define CHAT_ID     "1790655432"
// ─────────────────────────────────────────────────────────────────────────────

// Mirror everything to USB Serial so we can watch locally too
TelegramSerial tg(WIFI_SSID, WIFI_PASS, BOT_TOKEN, CHAT_ID, &Serial);

// ── Test state ────────────────────────────────────────────────────────────────
static int passed = 0, failed = 0;
static void tPASS(const char* name) { passed++; Serial.printf("  [PASS] %s\n", name); }
static void tFAIL(const char* name, const char* reason) {
    failed++;
    Serial.printf("  [FAIL] %s — %s\n", name, reason);
}

void runTests() {
    Serial.println("\n=== TelegramSerial Test Suite ===");
    tg.send("=== TelegramSerial Test Suite ===");

    // ── Test 1: WiFi connected ────────────────────────────────────────────────
    if (tg.connected()) tPASS("WiFi connected");
    else                tFAIL("WiFi connected", "not connected after begin()");

    // ── Test 2: send() queues a message ──────────────────────────────────────
    bool ok = tg.send("Test 2: send() works");
    if (ok) tPASS("send() returns true");
    else    tFAIL("send()", "returned false");

    // ── Test 3: println() interface ───────────────────────────────────────────
    tg.println("Test 3: println() works");
    tPASS("println() — no crash");

    // ── Test 4: printf-style via print(buf) ──────────────────────────────────
    char buf[64];
    snprintf(buf, sizeof(buf), "Test 4: Chip=%s Heap=%u", 
             ESP.getChipModel(), ESP.getFreeHeap());
    tg.println(buf);
    tPASS("printf-style via snprintf+println");

    // ── Test 5: Queue burst (8 messages at once) ──────────────────────────────
    Serial.println("  Queuing 8 messages...");
    for (int i = 1; i <= 8; i++) {
        char msg[32];
        snprintf(msg, sizeof(msg), "Burst msg %d/8", i);
        tg.send(msg);
    }
    uint8_t q = tg.queued();
    Serial.printf("  Queue depth: %d\n", q);
    if (q > 0) tPASS("queue burst — messages queued");
    else        tFAIL("queue burst", "queue empty immediately (unexpected)");

    // ── Test 6: connected() matches WiFi.status() ────────────────────────────
    bool connCheck = (tg.connected() == (WiFi.status() == WL_CONNECTED));
    if (connCheck) tPASS("connected() matches WiFi.status()");
    else            tFAIL("connected()", "mismatch with WiFi.status()");

    // ── Summary ───────────────────────────────────────────────────────────────
    Serial.printf("\n  Local result: %d passed, %d failed\n", passed, failed);
    Serial.println("  Watching Telegram for burst delivery...");
    Serial.println("  (30s heartbeat will follow)\n");

    char summary[80];
    snprintf(summary, sizeof(summary),
             "Tests done: %d passed, %d failed. Heap=%u bytes",
             passed, failed, ESP.getFreeHeap());
    tg.send(summary);
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.printf("\nChip: %s  Rev: %d  Flash: %uMB\n",
                  ESP.getChipModel(), ESP.getChipRevision(),
                  ESP.getFlashChipSize() / (1024*1024));

    if (!tg.begin()) {
        Serial.println("[WARN] WiFi connect failed — messages will queue");
    }

    runTests();
}

unsigned long lastHeartbeat = 0;
int heartbeatCount = 0;

void loop() {
    tg.update();  // drain queue — must be called every loop

    // Heartbeat every 30s — confirms sustained operation
    if (millis() - lastHeartbeat >= 30000) {
        lastHeartbeat = millis();
        heartbeatCount++;
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "Heartbeat #%d — uptime=%lus heap=%u queued=%d",
                 heartbeatCount, millis()/1000,
                 ESP.getFreeHeap(), tg.queued());
        tg.send(msg);
        Serial.printf("[heartbeat] %s\n", msg);

        // Stop after 3 heartbeats
        if (heartbeatCount >= 3) {
            tg.send("Test complete. All done!");
            Serial.println("\nTest complete. All done.");
            while (true) { tg.update(); delay(500); }
        }
    }
}
