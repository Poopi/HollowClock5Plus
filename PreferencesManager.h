#ifndef _PREFERENCEMANAGER_H_
#define _PREFERENCEMANAGER_H_

#include "config.h"
#include <Preferences.h>

typedef enum {
  PREF_OK = 0,
  PREF_ERROR = -1,
} pref_result_t;

class PreferencesManager {
private:
  const unsigned char PREFS_CURRENT_VERSION = 1;

public:
  static PreferencesManager &getInstance();
  PreferencesManager(const PreferencesManager &) = delete;
  PreferencesManager &operator=(const PreferencesManager &) = delete;

  void printPreferences(void);
  void eraseAll(void);

  String getHostName(void);
  pref_result_t setHostName(const String &hostname);

  String getNTPServer(void);
  pref_result_t setNTPServer(const String &ntpServer);

  String getTimeZone(void);
  pref_result_t setTimeZone(const String &timezone);

  String getTimeZoneLocation(void);
  pref_result_t setTimeZoneLocation(const String &timezone);

  bool getManualTimezone(void);
  pref_result_t setManualTimezone(bool manual);

  int getManualTimezoneValue(void);
  pref_result_t setManualTimezoneValue(int timezone);

  bool getFlipRotation(void);
  pref_result_t setFlipRotation(bool flip);

  bool getAllowBackward(void);
  pref_result_t setAllowBackward(bool allow);

  uint32_t getStepsPerMinute(void);
  pref_result_t setStepsPerMinute(uint32_t steps);

  uint8_t getDelayTime(void);
  pref_result_t setDelayTime(uint8_t delay);

  String getServerIP(void);
  pref_result_t setServerIP(const String &ip);

  String getServerGW(void);
  String getServerMask(void);

  String getSSID(void);
  pref_result_t setSSID(const String &ssid);

  String getPassword(void);
  pref_result_t setPassword(const String &password);

  int32_t getClockPosition(void);
  pref_result_t setClock(int32_t position);

  uint32_t getNTPUpdate(void);
  pref_result_t setNTPUpdate(uint32_t update);

  static const int32_t INVALID_CLOCK_POSITION = -1;

private:
  PreferencesManager();
  ~PreferencesManager();

  void writeInitialSettings(void);
  void readAllSettings(void);

  const char *prefs_version_key = "Version" PROGMEM;
  const char *prefs_server_hostname_key = "Hostname" PROGMEM;
  const char *prefs_server_ip_key = "ServerIP" PROGMEM;
  const char *prefs_ssid_key = "SSID" PROGMEM;
  const char *prefs_password_key = "Password" PROGMEM;
  const char *prefs_ntpserver_key = "NtpServer" PROGMEM;
  const char *prefs_ntp_timeout_key = "NtpTimeout" PROGMEM;
  const char *prefs_timezone_key = "TZ" PROGMEM;
  const char *prefs_timezone_location_key = "TZLocation" PROGMEM;
  const char *prefs_tz_manual_key = "TZManual" PROGMEM;
  const char *prefs_tz_manual_value_key = "TZManualValue" PROGMEM;
  const char *prefs_flip_rotation_key = "FlipRotation" PROGMEM;
  const char *prefs_allow_backward_key = "AllowBackward" PROGMEM;
  const char *prefs_steps_per_minute_key = "StepsPm" PROGMEM;
  const char *prefs_delay_time_key = "DelayTime" PROGMEM;
  const char *prefs_clock_position_key = "ClockPos" PROGMEM;

  String server_ip = "192.168.100.1" PROGMEM;
  String server_gw = "192.168.100.1" PROGMEM;
  String server_mask = "255.255.255.0" PROGMEM;

  String server_hostname = "Hollow5Plus" PROGMEM;
  String ssid = "" PROGMEM;
  String password = "" PROGMEM;
  String ntpserver = DEFAULT_NTP_SERVER PROGMEM;
  String timezone = DEFAULT_TIMEZONE PROGMEM;
  String timezone_location = DEFAULT_TIMEZONE_LOCATION PROGMEM;
  bool timezone_manual = false;
  int timezone_manual_value = 0;
  bool flip_rotation = false;
  bool allow_backward = false;
  uint32_t steps_per_minute = 256;
  uint8_t delay_time = 2;
  uint32_t ntp_update = 60 * 60 * 12;
  int32_t clock_position = INVALID_CLOCK_POSITION;

  Preferences preferences;
};

#endif // PREFS_H
