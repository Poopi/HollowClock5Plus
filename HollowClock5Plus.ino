#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <nvs_flash.h>
#include <time.h>

#include "ClockWebServer.h"
#include "HollowClock.h"
#include "MotorControl.h"
#include "PreferencesManager.h"
#include "SoundPlayer.h"
#include "config.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "Zones.h"

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
    ERROR("Failed to obtain time\n");
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
}

void wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info) {
  TRACE("Connected to AP successfully!\n");
}
void wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  TRACE("Disconnected from AP! Reson:%d\n", info.wifi_sta_disconnected.reason);
  PreferencesManager &pm = PreferencesManager::getInstance();
  String ssid = pm.getSSID();
  String password = pm.getPassword();
  WiFi.begin(ssid.c_str(), password.c_str());
  sntp_restart();
}

void wifi_got_ip(WiFiEvent_t event, WiFiEventInfo_t info) {
  TRACE("WiFi IP:%s\n", WiFi.localIP().toString().c_str());
  TRACE("\t Mask:%s\n", WiFi.subnetMask().toString().c_str());
  TRACE("\t GW:%s\n", WiFi.gatewayIP().toString().c_str());
}

bool connectToNetwork(void) {
  PreferencesManager &pm = PreferencesManager::getInstance();
  DBG(Serial.print("Connecting to "));
  String ssid = pm.getSSID();
  DBG(Serial.println(ssid));
  String password = pm.getPassword();
  WiFiEventId_t evenIDs[3];

  if (ssid.isEmpty()) {
    Serial.println("SSID is empty, cannot connect to network.");
    return false;
  }
  // Apparently this has to be the first WiFi call in order to work
  WiFi.hostname(pm.getHostName());
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  evenIDs[0] =
      WiFi.onEvent(wifi_got_ip, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  evenIDs[1] = WiFi.onEvent(wifi_connected,
                            WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  evenIDs[2] = WiFi.onEvent(wifi_disconnected,
                            WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(ssid.c_str(), password.c_str());

  byte attempts = WIFI_MAX_ATTEMPTS;
  wl_status_t status;
  do {
    status = WiFi.status();
    delay(WIFI_DELAY);
    DBG(Serial.print("."));
  } while ((status != WL_CONNECTED) && (--attempts > 0));

  if (status != WL_CONNECTED) {
    WiFi.removeEvent(evenIDs[0]);
    WiFi.removeEvent(evenIDs[1]);
    WiFi.removeEvent(evenIDs[2]);
    WiFi.disconnect(true);
  } else {
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleep(false);
  }
  return status == WL_CONNECTED;
}

void sync_time_cb(struct timeval *t) {
  TRACE("Time synced from NTP!\n");
  tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
    HollowClock::getInstance().setLastSyncedTime(String(time_str));
    DBG(printLocalTime());
  }
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
#if DEBUG
  Serial.setDebugOutput(true);
#else
  Serial.setDebugOutput(false);
#endif
#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
  esp_sleep_enable_timer_wakeup(1); // uS micro seconds
#endif
  PreferencesManager &pm = PreferencesManager::getInstance();
  MotorControl &motor = MotorControl::getInstance();
  pm.printPreferences();
  HollowClock &hclock = HollowClock::getInstance();
  hostname = pm.getHostName();
  if (connectToNetwork()) {
    String ip_addr = WiFi.localIP().toString();
    TRACE("WiFi connected. IP address: %s\n",
          WiFi.localIP().toString().c_str());
    SoundPlayer::getInstance().playMusic(MUSIC_NOKIA_RINGTONE);
    MDNS.begin(hostname);
    wifi_setup_done = true;
  } else {
    TRACE("AP:%s\n", getChipDefaultSSID().c_str());
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(pm.getServerIP().c_str(), pm.getServerGW().c_str(),
                      pm.getServerMask().c_str());
    WiFi.softAP(getChipDefaultSSID().c_str());
    WiFi.setHostname(pm.getHostName().c_str());
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  }

  TRACE("WiFi acting as %s\n", wifi_setup_done ? "STA" : "AP");
  // The timezone must be in place before the clock thread reads the time.
  // After a reset (not a power loss) the RTC still holds a valid time, and a
  // thread started earlier would read it as UTC and move the hands by hours.
  initTime();
  hclock.start();
  ClockWebServer &clockWebServer = ClockWebServer::getInstance();
  clockWebServer.start();
}

void initTime() {
  TRACE("Init NTP\n");
  PreferencesManager &pm = PreferencesManager::getInstance();
  static String ntp_server = pm.getNTPServer();
  String time_zone = pm.getTimeZone();
  bool ntp_manual = pm.getManualTimezone();
  int timezone_offset = pm.getManualTimezoneValue();

  esp_netif_sntp_deinit();
  sntp_set_time_sync_notification_cb(sync_time_cb);
  if (ntp_manual) {
    configTime(-timezone_offset * 60, 0, ntp_server.c_str());
  } else {
    configTzTime(time_zone.c_str(), ntp_server.c_str());
  }

  sntp_set_sync_interval(pm.getNTPUpdate() * 1000UL);
  sntp_restart();
}

void loop() {
  ClockWebServer &clockWebServer = ClockWebServer::getInstance();
  clockWebServer.handleClient();
  delay(1);
}
