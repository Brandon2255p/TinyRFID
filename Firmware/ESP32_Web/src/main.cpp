#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "EM4100.h"
#include "web_content.h"

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
#ifndef EM4100_TX_PIN
#define EM4100_TX_PIN 4
#endif

// Default sweep range, matching the original DigiSpark firmware
// (half-bit delay swept from 200us to 220us).
static const uint16_t DEFAULT_SWEEP_MIN = 200;
static const uint16_t DEFAULT_SWEEP_MAX = 220;
static const uint16_t DEFAULT_REPEAT    = 20;   // frames per delay step

// Soft WiFi access point. Change if you like.
static const char *AP_SSID = "TinyRFID";
static const char *AP_PASS = "tinyrfid123";   // >= 8 chars, or "" for open AP

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
EM4100 em4100;
WebServer server(80);

enum TxMode { MODE_SWEEP = 0, MODE_FIXED = 1 };

volatile bool txActive = false;
uint8_t  txData[5]    = {0x30, 0x7F, 0xA2, 0x53, 0x5A}; // demo default
char     txIdHex[11]  = "307FA2535A";
TxMode   txMode       = MODE_SWEEP;
uint16_t sweepMin     = DEFAULT_SWEEP_MIN;
uint16_t sweepMax     = DEFAULT_SWEEP_MAX;
uint16_t fixedDelay   = 200;
uint16_t repeatPerStep = DEFAULT_REPEAT;

// Runtime sweep cursor
uint16_t curDelay    = DEFAULT_SWEEP_MIN;
uint16_t framesSent  = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Parse a 10-character hex string into 5 bytes. Returns true on success.
static bool parseHexId(const String &hex, uint8_t out[5], char normalized[11])
{
  if (hex.length() != 10)
    return false;

  for (int i = 0; i < 10; i++)
  {
    char c = hex[i];
    uint8_t v;
    if (c >= '0' && c <= '9')      v = c - '0';
    else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
    else return false;

    if ((i & 1) == 0) out[i / 2]  = v << 4;
    else              out[i / 2] |= v;

    normalized[i] = (c >= 'a' && c <= 'f') ? (char)(c - 32) : c;
  }
  normalized[10] = '\0';
  return true;
}

static uint16_t clampDelay(long v)
{
  if (v < 50)   v = 50;    // guard against absurd / unsafe values
  if (v > 2000) v = 2000;
  return (uint16_t)v;
}

// ---------------------------------------------------------------------------
// HTTP handlers
// ---------------------------------------------------------------------------
static void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}

static void sendStatusJson()
{
  String json = "{";
  json += "\"active\":" + String(txActive ? "true" : "false");
  json += ",\"id\":\"" + String(txIdHex) + "\"";
  json += ",\"mode\":\"" + String(txMode == MODE_SWEEP ? "sweep" : "fixed") + "\"";
  json += ",\"delay\":" + String(curDelay);
  json += ",\"sweepMin\":" + String(sweepMin);
  json += ",\"sweepMax\":" + String(sweepMax);
  json += ",\"fixedDelay\":" + String(fixedDelay);
  json += ",\"repeat\":" + String(repeatPerStep);
  json += "}";
  server.send(200, "application/json", json);
}

static void handleStatus()
{
  sendStatusJson();
}

static void handleTransmit()
{
  String id = server.arg("id");
  uint8_t data[5];
  char norm[11];
  if (!parseHexId(id, data, norm))
  {
    server.send(400, "application/json",
                "{\"ok\":false,\"error\":\"id must be 10 hex characters\"}");
    return;
  }

  memcpy(txData, data, 5);
  strcpy(txIdHex, norm);

  String mode = server.arg("mode");
  txMode = (mode == "fixed") ? MODE_FIXED : MODE_SWEEP;

  if (server.hasArg("sweepMin")) sweepMin = clampDelay(server.arg("sweepMin").toInt());
  if (server.hasArg("sweepMax")) sweepMax = clampDelay(server.arg("sweepMax").toInt());
  if (server.hasArg("fixedDelay")) fixedDelay = clampDelay(server.arg("fixedDelay").toInt());
  if (server.hasArg("repeat"))
  {
    long r = server.arg("repeat").toInt();
    if (r < 1) r = 1;
    if (r > 1000) r = 1000;
    repeatPerStep = (uint16_t)r;
  }

  if (sweepMax < sweepMin)
  {
    uint16_t t = sweepMin; sweepMin = sweepMax; sweepMax = t;
  }

  // Prime the state machine
  framesSent = 0;
  if (txMode == MODE_SWEEP)
  {
    curDelay = sweepMin;
    em4100.setDelayUs(curDelay);
  }
  else
  {
    curDelay = fixedDelay;
    em4100.setDelayUs(curDelay);
  }

  txActive = true;
  sendStatusJson();
}

static void handleStop()
{
  txActive = false;
  sendStatusJson();
}

static void handleNotFound()
{
  server.send(404, "text/plain", "Not found");
}

// ---------------------------------------------------------------------------
// Arduino lifecycle
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(100);

  em4100.setTxPin(EM4100_TX_PIN);
  em4100.setDelayUs(curDelay);

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, strlen(AP_PASS) >= 8 ? AP_PASS : nullptr);
  IPAddress ip = WiFi.softAPIP();

  Serial.println();
  Serial.println("TinyRFID ESP32 EM4100 emulator");
  Serial.printf("AP \"%s\" %s\n", AP_SSID, ok ? "started" : "FAILED");
  Serial.print("Open http://");
  Serial.print(ip);
  Serial.println("/");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/transmit", HTTP_POST, handleTransmit);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop()
{
  server.handleClient();

  if (!txActive)
    return;

  // Transmit exactly one frame per loop iteration so the web server stays
  // responsive (one frame is ~25ms). After `repeatPerStep` frames, advance
  // the sweep — this reproduces the original firmware's behavior of sending
  // a card 20x per half-bit delay and sweeping 200..220us.
  em4100.transmitEM4100(txData);
  framesSent++;

  if (framesSent >= repeatPerStep)
  {
    framesSent = 0;
    if (txMode == MODE_SWEEP)
    {
      curDelay++;
      if (curDelay > sweepMax)
        curDelay = sweepMin;
      em4100.setDelayUs(curDelay);
    }
    // MODE_FIXED keeps curDelay constant.
  }
}
