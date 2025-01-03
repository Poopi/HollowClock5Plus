#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <nvs_flash.h>
#include <time.h>

#include "ClockWebServer.h"
#include "HollowClock.h"
#include "PreferencesManager.h"
#include "config.h"
#include "esp_sntp.h"
#include "zones.h"

#if DEBUG
#define DBG(x) x
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#else
#define DBG(x)
#define TRACE(...)
#define ERROR(...)
#endif

#define WIFI_MAX_ATTEMPTS 60 // 60*500ms - 30 secs
#define WIFI_DELAY 500       // ms

DNSServer dnsServer;

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    ERROR("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}

bool connectToNetwork(void) {
  PreferencesManager &pm = PreferencesManager::getInstance();
  DBG(Serial.print("Connecting to "));
  String ssid = pm.getSSID();
  DBG(Serial.println(ssid));
  String password = pm.getPassword();

  if (ssid.isEmpty()) {
    Serial.println("SSID is empty, cannot connect to network.");
    return false;
  }

  WiFi.begin(ssid.c_str(), password.c_str());
  byte attempts = WIFI_MAX_ATTEMPTS;
  wl_status_t status;
  do {
    status = WiFi.status();
    delay(WIFI_DELAY);
    DBG(Serial.print("."));
  } while ((status != WL_CONNECTED) && (--attempts > 0));

  if (status != WL_CONNECTED) {
    WiFi.disconnect();
  } else {
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }
  return status == WL_CONNECTED;
}

void getNTPTime(void) {
  String ntp_server;
  String time_zone;
  bool ntp_manual;
  int timezone_offset;

  PreferencesManager &pm = PreferencesManager::getInstance();

  ntp_server = pm.getNTPServer();
  time_zone = pm.getTimeZone();
  ntp_manual = pm.getManualTimezone();
  timezone_offset = pm.getManualTimezoneValue();

  if (ntp_manual) {
    configTime(-timezone_offset * 60, 0, ntp_server.c_str());
  } else {
    configTzTime(time_zone.c_str(), ntp_server.c_str());
  }
  sntp_set_sync_interval(pm.getNTPUpdate() * 1000UL);
  DBG(printLocalTime();)
}

String getChipDefaultSSID(void) {
  uint64_t chipid = ESP.getEfuseMac() >> 40;
  String hex = String(chipid, HEX);
  hex.toUpperCase();
  return DEFAULT_AP_NAME_PREFIX + hex;
}

void setup() {
  bool wifi_setup_done = false;
  String hostname;

  Serial.begin(SERIAL_BAUD_RATE);
#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
  esp_sleep_enable_timer_wakeup(1); // uS micro seconds
#endif
  PreferencesManager &pm = PreferencesManager::getInstance();
  pm.printPreferences();
  HollowClock &hclock = HollowClock::getInstance();
  ClockWebServer &clockWebServer = ClockWebServer::getInstance();

  pinMode(LED_BUILTIN, OUTPUT);
  hostname = pm.getHostName();

  if (connectToNetwork()) {
    String ip_addr = WiFi.localIP().toString();
    TRACE("WiFi connected. IP address: %s\n",
          WiFi.localIP().toString().c_str());
    MDNS.begin(hostname);
    WiFi.hostname(hostname);
    getNTPTime();
    hclock.start();
    wifi_setup_done = true;
  } else {
    TRACE("AP:%s\n", getChipDefaultSSID().c_str());
    WiFi.softAPConfig(pm.getServerIP().c_str(), pm.getServerGW().c_str(),
                      pm.getServerMask().c_str());
    WiFi.softAP(pm.getHostName().c_str());
    WiFi.hostname(pm.getHostName().c_str());
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    WiFi.persistent(true);
  }

  TRACE("WiFi acting as %s\n", wifi_setup_done ? "STA" : "AP");
  clockWebServer.start();
}

void loop() {
  ClockWebServer &clockWebServer = ClockWebServer::getInstance();
  clockWebServer.handleClient();
  delay(1);
}
