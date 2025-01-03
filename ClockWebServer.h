#ifndef WEBSRVR_H
#define WEBSRVR_H

#include <Preferences.h>
#include <WebServer.h>

class ClockWebServer {
public:
  static ClockWebServer &getInstance();
  void start();
  void setPreferenceHandle(Preferences *prefs);
  void handleClient();
  void send(int code, const char *content_type, const String &data);

private:
  ClockWebServer() = default;
  WebServer *webServer;
  Preferences *prefs;
  String lastError = "";

  void setServerRouting();
  void handleRoot();
  void handleStyles();
  void handleWifi();
  void handleTime();
  void handleAdvanced();
  void handlePosition();
  void handleWifiGet();
  void handleWifiPost();
  void handleSsidListGet();
  void handleTimeGet();
  void handleTimePost();
  void handleAdvancedGet();
  void handleAdvancedPost();
  void handleCalibrationPost();
  void handlePositionGet();
  void handleApplyPost();
  void handleResetPost();
  void handleError();

  void sendError(const String &message);
};

#endif // WEBSRVR_H
