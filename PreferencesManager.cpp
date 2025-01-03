#include "PreferencesManager.h"
#include <Preferences.h>
#include <nvs_flash.h>

#if DEBUG
#define DBG(x) x
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#else
#define DBG(x)
#define TRACE(...)
#define ERROR(...)
#endif

PreferencesManager &PreferencesManager::getInstance() {
  static PreferencesManager instance;
  return instance;
}

void PreferencesManager::printPreferences(void) {
  TRACE("Preferences:\n");
  TRACE("\tServer Hostname: %s\n", server_hostname.c_str());
  TRACE("\tSSID: %s\n", ssid.c_str());
  TRACE("\tPassword: %s\n", password.c_str());
  TRACE("\tNTP Server: %s\n", ntpserver.c_str());
  TRACE("\tTimezone: %s\n", timezone.c_str());
  TRACE("\tNTP Timeout: %d\n", ntp_update);
  TRACE("\tTimezone Location: %s\n", timezone_location.c_str());
  TRACE("\tManual Timezone: %s\n", timezone_manual ? "true" : "false");
  TRACE("\tManual Timezone Value: %d\n", timezone_manual_value);
  TRACE("\tFlip Rotation: %s\n", flip_rotation ? "true" : "false");
  TRACE("\tAllow Backward: %s\n", allow_backward ? "true" : "false");
  TRACE("\tSteps Per Minute: %d\n", steps_per_minute);
  TRACE("\tDelay Time: %d\n", delay_time);
  TRACE("\tServer IP: %s\n", server_ip.c_str());
  TRACE("\tClock Position: %d\n", clock_position);
  TRACE("\tServer Gateway: %s\n", server_gw.c_str());
  TRACE("\tServer Mask: %s\n", server_mask.c_str());
}

void PreferencesManager::eraseAll(void) {
  TRACE("Wiping flash....\n");
  esp_err_t error;
  error = nvs_flash_erase(); // erase the NVS partition and...
  if (error == ESP_OK) {
    error = nvs_flash_init(); // initialize the NVS partition.
    if (error != ESP_OK) {
      ERROR("Flash init status ....:%d\n", error);
    }
  } else {
    ERROR("Wiping flash error ....:%d\n");
  }
}

String PreferencesManager::getHostName(void) { return server_hostname; }

pref_result_t PreferencesManager::setHostName(const String &hostname) {
  if (hostname.isEmpty()) {
    ERROR("Hostname is empty\n");
    return PREF_ERROR;
  }
  if (server_hostname != hostname) {
    server_hostname = hostname;
    preferences.putString(prefs_server_hostname_key, hostname);
  }
  return PREF_OK;
}

String PreferencesManager::getNTPServer(void) { return ntpserver; }

pref_result_t PreferencesManager::setNTPServer(const String &ntpServer) {
  if (ntpserver != ntpServer) {
    ntpserver = ntpServer;
    preferences.putString(prefs_ntpserver_key, ntpServer);
  }
  return PREF_OK;
}

String PreferencesManager::getTimeZone(void) { return timezone; }

pref_result_t PreferencesManager::setTimeZone(const String &tz) {
  if (timezone != tz) {
    timezone = tz;
    preferences.putString(prefs_timezone_key, tz);
  }
  return PREF_OK;
}

String PreferencesManager::getSSID(void) { return ssid; }

pref_result_t PreferencesManager::setSSID(const String &network_ssid) {
  if (ssid != network_ssid) {
    ssid = network_ssid;
    preferences.putString(prefs_ssid_key, network_ssid);
  }
  return PREF_OK;
}

String PreferencesManager::getPassword(void) { return password; }

pref_result_t PreferencesManager::setPassword(const String &pwd) {
  if (password != pwd) {
    password = pwd;
    preferences.putString(prefs_password_key, pwd);
  }
  return PREF_OK;
}

String PreferencesManager::getTimeZoneLocation(void) {
  return timezone_location;
}

pref_result_t
PreferencesManager::setTimeZoneLocation(const String &tz_location) {
  if (timezone_location != tz_location) {
    timezone_location = tz_location;
    preferences.putString(prefs_timezone_location_key, tz_location);
  }
  return PREF_OK;
}

bool PreferencesManager::getManualTimezone(void) { return timezone_manual; }

pref_result_t PreferencesManager::setManualTimezone(bool manual) {
  if (timezone_manual != manual) {
    timezone_manual = manual;
    preferences.putBool(prefs_tz_manual_key, manual);
  }
  return PREF_OK;
}

int PreferencesManager::getManualTimezoneValue(void) {
  return timezone_manual_value;
}

pref_result_t PreferencesManager::setManualTimezoneValue(int tz_value) {
  if (tz_value < -12 * 60 * 60 || tz_value > 12 * 60 * 60) {
    ERROR("Invalid timezone value:%d\n", tz_value);
    return PREF_ERROR;
  }
  if (timezone_manual_value != tz_value) {
    timezone_manual_value = tz_value;
    preferences.putInt(prefs_tz_manual_value_key, tz_value);
  }
  return PREF_OK;
}

bool PreferencesManager::getFlipRotation(void) { return flip_rotation; }

pref_result_t PreferencesManager::setFlipRotation(bool flip) {
  if (flip_rotation != flip) {
    flip_rotation = flip;
    preferences.putBool(prefs_flip_rotation_key, flip);
  }
  return PREF_OK;
}

bool PreferencesManager::getAllowBackward(void) { return allow_backward; }
pref_result_t PreferencesManager::setAllowBackward(bool allow) {
  if (allow_backward != allow) {
    allow_backward = allow;
    preferences.putBool(prefs_allow_backward_key, allow);
  }
  return PREF_OK;
}

uint32_t PreferencesManager::getStepsPerMinute(void) {
  return steps_per_minute;
}

pref_result_t PreferencesManager::setStepsPerMinute(uint32_t steps) {
  if (steps_per_minute != steps) {
    steps_per_minute = steps;
    preferences.putUInt(prefs_steps_per_minute_key, steps);
  }
  return PREF_OK;
}

uint8_t PreferencesManager::getDelayTime(void) { return delay_time; }

pref_result_t PreferencesManager::setDelayTime(uint8_t delay) {
  if (delay < 2) {
    ERROR("Invalid delay time:%d\n", delay);
    return PREF_ERROR;
  }
  if (delay_time != delay) {
    delay_time = delay;
    preferences.putUChar(prefs_delay_time_key, delay_time);
  }
  return PREF_OK;
}

String PreferencesManager::getServerIP(void) { return server_ip; }

pref_result_t PreferencesManager::setServerIP(const String &ip) {
  IPAddress ipAddr;
  if (!ipAddr.fromString(ip)) {
    ERROR("Invalid IP address: %s\n", ip.c_str());
    return PREF_ERROR;
  }
  if (server_ip != ip) {
    server_ip = ip;
    preferences.putString(prefs_server_ip_key, ip);
  }
  return PREF_OK;
}

int32_t PreferencesManager::getClockPosition(void) { return clock_position; }

pref_result_t PreferencesManager::setClock(int32_t position) {
  if (clock_position != position) {
    clock_position = position;
    preferences.putUInt(prefs_clock_position_key, position);
  }
  return PREF_OK;
}

uint32_t PreferencesManager::getNTPUpdate(void) { return ntp_update; }

pref_result_t PreferencesManager::setNTPUpdate(uint32_t update) {
  if (ntp_update != update) {
    ntp_update = update;
    preferences.putUInt(prefs_ntp_timeout_key, update);
  }
  return PREF_OK;
}

String PreferencesManager::getServerGW(void) { return server_gw; }

String PreferencesManager::getServerMask(void) { return server_mask; }

void PreferencesManager::writeInitialSettings(void) {
  preferences.putUChar(prefs_version_key, PREFS_CURRENT_VERSION);
  preferences.putString(prefs_server_hostname_key, server_hostname);
  preferences.putString(prefs_ssid_key, ssid);
  preferences.putString(prefs_password_key, password);
  preferences.putString(prefs_ntpserver_key, ntpserver);
  preferences.putString(prefs_timezone_key, timezone);
  preferences.putUInt(prefs_ntp_timeout_key, ntp_update);
  preferences.putString(prefs_timezone_location_key, timezone_location);
  preferences.putBool(prefs_tz_manual_key, timezone_manual);
  preferences.putInt(prefs_tz_manual_value_key, timezone_manual_value);
  preferences.putBool(prefs_flip_rotation_key, flip_rotation);
  preferences.putBool(prefs_allow_backward_key, allow_backward);
  preferences.putUInt(prefs_steps_per_minute_key, steps_per_minute);
  preferences.putUChar(prefs_delay_time_key, delay_time);
  preferences.putString(prefs_server_ip_key, server_ip);
  preferences.putUInt(prefs_clock_position_key, clock_position);
}

void PreferencesManager::readAllSettings() {
  server_hostname = preferences.getString(prefs_server_hostname_key);
  ssid = preferences.getString(prefs_ssid_key);
  password = preferences.getString(prefs_password_key);
  ntpserver = preferences.getString(prefs_ntpserver_key);
  timezone = preferences.getString(prefs_timezone_key);
  ntp_update = preferences.getUInt(prefs_ntp_timeout_key);
  timezone_location = preferences.getString(prefs_timezone_location_key);
  timezone_manual = preferences.getBool(prefs_tz_manual_key);
  timezone_manual_value = preferences.getInt(prefs_tz_manual_value_key);
  flip_rotation = preferences.getBool(prefs_flip_rotation_key);
  allow_backward = preferences.getBool(prefs_allow_backward_key);
  steps_per_minute = preferences.getUInt(prefs_steps_per_minute_key);
  delay_time = preferences.getUChar(prefs_delay_time_key);
  server_ip = preferences.getString(prefs_server_ip_key);
  clock_position = preferences.getUInt(prefs_clock_position_key);
}
PreferencesManager::PreferencesManager() {
  // Constructor implementation
  preferences.begin("HC5Plus", false);
  bool prefs_init = preferences.isKey(prefs_version_key);
  if (prefs_init == true) {
    unsigned char pref_version = preferences.getUChar(prefs_version_key);
    TRACE("Prefs version:%d\n", pref_version);

    if (pref_version != PREFS_CURRENT_VERSION) {
      // ToDo conversion
      // Wipe all for now
      eraseAll();
      delay(500);
      ERROR("Rebooting....\n");
      ESP.restart();
    } else {
      readAllSettings();
      DBG(printPreferences());
    }
  } else {
    TRACE("Initialize prefs\n");
    writeInitialSettings();
  }
}

PreferencesManager::~PreferencesManager() {
  // Destructor implementation
  preferences.end();
}
