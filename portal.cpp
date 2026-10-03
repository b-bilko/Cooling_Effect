#include "portal.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "app.h"
#include "portal_page.h"

// Reachable as http://cooling-effect.local when on the same network
#define MDNS_NAME "cooling-effect"

// A browser counts as connected for this long after its last request
#define PORTAL_IDLE_MS 10000

static const byte DNS_PORT = 53;

static WebServer server(80);
static DNSServer dns;

static bool     seenRequest = false;
static uint32_t lastRequestMs = 0;
static bool     rebootPending = false;
static uint32_t rebootAtMs = 0;

static void touch() {
  seenRequest = true;
  lastRequestMs = millis();
}

static String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 2);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if ((uint8_t)c < 0x20) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\\u%04x", c);
      out += buf;
    } else {
      out += c;
    }
  }
  return out;
}

static void sendState() {
  FadeStatus st = fadeStatus();
  String staIp = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";

  String json = "{";
  json += "\"heat\":" + String(st.heat);
  json += ",\"percent\":" + String(st.percent);
  json += ",\"cooling\":" + String(st.cooling ? "true" : "false");
  json += ",\"secondsLeft\":" + String(st.secondsLeft);
  json += ",\"sleepInSeconds\":" + String(st.sleepInSeconds);
  json += ",\"fadeSeconds\":" + String(settings.fadeSeconds);
  json += ",\"startHeat\":" + String(settings.startHeat);
  json += ",\"brightness\":" + String(settings.brightness);
  json += ",\"maxMilliamps\":" + String(settings.maxMilliamps);
  json += ",\"sleepDelaySeconds\":" + String(settings.sleepDelaySeconds);
  json += ",\"apSsid\":\"" + jsonEscape(WiFi.softAPSSID()) + "\"";
  json += ",\"wifiSsid\":\"" + jsonEscape(settings.wifiSsid) + "\"";
  json += ",\"staIp\":\"" + staIp + "\"";
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// Reads an integer form field, clamped to [lo, hi]; keeps `current` if absent
static long argInt(const char *name, long current, long lo, long hi) {
  if (!server.hasArg(name)) {
    return current;
  }
  return constrain(server.arg(name).toInt(), lo, hi);
}

static void handleRoot() {
  touch();
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", PORTAL_PAGE);
}

static void handleState() {
  touch();
  sendState();
}

static void handleSettings() {
  touch();
  settings.fadeSeconds       = argInt("fadeSeconds", settings.fadeSeconds, FADE_SECONDS_MIN, FADE_SECONDS_MAX);
  settings.startHeat         = argInt("startHeat", settings.startHeat, START_HEAT_MIN, START_HEAT_MAX);
  settings.brightness        = argInt("brightness", settings.brightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  settings.maxMilliamps      = argInt("maxMilliamps", settings.maxMilliamps, MAX_MILLIAMPS_MIN, MAX_MILLIAMPS_MAX);
  settings.sleepDelaySeconds = argInt("sleepDelaySeconds", settings.sleepDelaySeconds, SLEEP_DELAY_MIN, SLEEP_DELAY_MAX);
  saveSettings(settings);
  applySettings();
  sendState();
}

static void handleRestart() {
  touch();
  restartFade();
  sendState();
}

static void handleSleep() {
  touch();
  server.send(200, "text/plain", "Going to sleep. Press the button to wake.");
  requestSleep();
}

// Wi-Fi changes need a reboot. Blank password fields keep the current value.
static void handleWifi() {
  touch();
  String apPass   = server.arg("apPassword");
  String ssid     = server.arg("wifiSsid");
  String wifiPass = server.arg("wifiPassword");
  ssid.trim();

  if (apPass.length() && (apPass.length() < AP_PASSWORD_MIN_LEN ||
                          apPass.length() > WIFI_PASSWORD_MAX_LEN)) {
    server.send(400, "text/plain", "Access point password must be 8-63 characters.");
    return;
  }
  if (ssid.length() > WIFI_SSID_MAX_LEN || wifiPass.length() > WIFI_PASSWORD_MAX_LEN) {
    server.send(400, "text/plain", "Network name or password is too long.");
    return;
  }

  if (apPass.length()) {
    settings.apPassword = apPass;
  }
  // A new network takes the typed password (blank = open network); the same
  // network keeps its saved password unless a new one is typed.
  if (ssid != settings.wifiSsid || wifiPass.length()) {
    settings.wifiPassword = wifiPass;
  }
  settings.wifiSsid = ssid;
  saveSettings(settings);

  server.send(200, "text/plain", "Saved. Rebooting to apply Wi-Fi settings...");
  rebootPending = true;
  rebootAtMs = millis() + 1000;
}

// Send unknown URLs (including OS captive-portal probes) to the main page
static void handleNotFound() {
  touch();
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void portalBegin() {
  char apSsid[32];
  uint64_t mac = ESP.getEfuseMac();
  snprintf(apSsid, sizeof(apSsid), "CoolingEffect-%04X", (uint16_t)(mac >> 32));

  bool joinHome = settings.wifiSsid.length() > 0;
  WiFi.persistent(false);
  WiFi.setHostname(MDNS_NAME);  // must precede WiFi.mode()
  WiFi.mode(joinHome ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(apSsid, settings.apPassword.c_str());
  if (joinHome) {
    WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPassword.c_str());
  }

  dns.start(DNS_PORT, "*", WiFi.softAPIP());
  MDNS.begin(MDNS_NAME);
  MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.on("/api/sleep", HTTP_POST, handleSleep);
  server.on("/api/wifi", HTTP_POST, handleWifi);
  server.onNotFound(handleNotFound);
  server.begin();
}

void portalLoop() {
  dns.processNextRequest();
  server.handleClient();

  if (rebootPending && (int32_t)(millis() - rebootAtMs) >= 0) {
    ESP.restart();
  }
}

void portalEnd() {
  server.stop();
  dns.stop();
  MDNS.end();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool portalActive() {
  if (WiFi.softAPgetStationNum() > 0) {
    return true;
  }
  return seenRequest && millis() - lastRequestMs < PORTAL_IDLE_MS;
}
