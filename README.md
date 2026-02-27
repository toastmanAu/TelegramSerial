# TelegramSerial

Drop-in `Serial` replacement for ESP32 that sends output to a Telegram bot over WiFi.

Inherits from `Print` — works anywhere `Serial` does. Just swap `Serial` for `tg`.

> **Each user creates their own free Telegram bot** via [@BotFather](https://t.me/BotFather).
> Your serial output goes directly from your device to your chat — no relay server,
> no third party, no shared infrastructure. Fully segmented and private by design.

## Features

- **Drop-in** — inherits `Print`, so `print()`, `println()`, `printf()` all work
- **Non-blocking** — queues messages, drains one per `update()` call
- **Rate-limited** — respects Telegram's API limits automatically
- **WiFi resilient** — auto-reconnects, retries failed sends
- **Mirror mode** — optionally echo to hardware Serial simultaneously
- **Markdown mode** — wrap output in monospace code blocks
- **Configurable** — queue size, rate limit, timeouts all adjustable via `#define`

## Installation

**Arduino Library Manager:** search for `TelegramSerial`

**Manual:** download and place in `~/Arduino/libraries/TelegramSerial/`

**PlatformIO:** add to `platformio.ini`:
```ini
lib_deps = toastmanAu/TelegramSerial
```

## Quick Start

```cpp
#include <TelegramSerial.h>

TelegramSerial tg("MyNetwork", "password", "BOT_TOKEN", "CHAT_ID");

void setup() {
    Serial.begin(115200);
    tg.begin();              // connect WiFi
    tg.println("Hello!");    // queued, sent on next update()
}

void loop() {
    tg.update();             // call every loop — drains queue
}
```

## Setup: Creating Your Bot

You need two things: a **bot token** and a **chat ID**. Takes about 2 minutes.

### Step 1 — Create a bot (get the token)

1. Open Telegram and search for **@BotFather** (official, blue tick)
2. Send `/newbot`
3. Choose a display name (e.g. `My ESP32 Bot`)
4. Choose a username — must end in `bot` (e.g. `myesp32_bot`)
5. BotFather replies with your token — looks like `123456789:ABC-xyz...`

Copy that token. It goes in `BOT_TOKEN` in your sketch.

### Step 2 — Get your chat ID

**Option A — Personal DM (simplest):**
1. Search for your bot by username and press **Start**
2. Visit this URL in a browser (replace `YOUR_TOKEN`):
   ```
   https://api.telegram.org/botYOUR_TOKEN/getUpdates
   ```
3. Send any message to your bot, refresh the URL
4. Find `"chat":{"id":123456789}` — that number is your chat ID

**Option B — Group chat:**
1. Create a group (or use an existing one)
2. Add your bot to the group
3. Send any message in the group
4. Visit the `getUpdates` URL above — the chat ID will be a **negative** number (e.g. `-1001234567890`)

**Option C — Use @userinfobot:**
1. Add [@userinfobot](https://t.me/userinfobot) to your group
2. It will reply with the group's chat ID
3. Remove it when done

### Step 3 — Use in your sketch

```cpp
#define BOT_TOKEN  "123456789:ABC-YourTokenHere"
#define CHAT_ID    "123456789"        // personal DM — positive number
// or
#define CHAT_ID    "-1001234567890"   // group chat — negative number

TelegramSerial tg(WIFI_SSID, WIFI_PASS, BOT_TOKEN, CHAT_ID);
```

> **Note:** Your bot must have received at least one message before `getUpdates` will return anything. If the list is empty, send your bot a message first then refresh.

## Constructor

```cpp
TelegramSerial tg(ssid, password, botToken, chatId);
TelegramSerial tg(ssid, password, botToken, chatId, &Serial);         // mirror to Serial
TelegramSerial tg(ssid, password, botToken, chatId, &Serial, TG_FMT_MARKDOWN); // monospace
```

## API

| Method | Description |
|---|---|
| `begin()` | Connect WiFi (blocking up to `TG_WIFI_TIMEOUT_MS`). Returns `true` if connected |
| `update()` | Drain send queue — call every `loop()` |
| `send(msg)` | Queue a message directly (bypasses line buffer) |
| `flushLine()` | Force-flush current line buffer without waiting for `\n` |
| `connected()` | Returns `true` if WiFi is up |
| `queued()` | Number of messages currently waiting to send |
| `print()` / `println()` / `printf()` | Standard Print interface — same as Serial |

## Configuration

Override before `#include <TelegramSerial.h>` or via `build_flags`:

```cpp
#define TG_LINE_BUF_SIZE      512    // max chars per message (default 512)
#define TG_QUEUE_SIZE          16    // max queued messages (default 16)
#define TG_SEND_INTERVAL_MS  1200    // min ms between sends (default 1200)
#define TG_WIFI_TIMEOUT_MS  12000    // WiFi connect timeout (default 12000)
#define TG_MAX_MSG_RETRIES      2    // retries before dropping a message (default 2)
```

## Drop-in Replacement Pattern

Any sketch that uses a configurable output target:

```cpp
#include <TelegramSerial.h>

TelegramSerial tg("ssid", "pass", "TOKEN", "CHAT_ID", &Serial);

// Change one line — everything else unchanged
#define MY_OUTPUT tg   // was: #define MY_OUTPUT Serial
```

## Notes

- Uses HTTPS (`WiFiClientSecure`) with certificate validation skipped — fine for telemetry
- Messages longer than `TG_LINE_BUF_SIZE` are truncated to fit
- If the queue fills up, the oldest message is dropped to make room for new ones
- `update()` is non-blocking — max one Telegram send per call, so your loop stays responsive
- WiFi reconnect is attempted in `update()` but won't block if it fails

## License

MIT © toastmanAu
