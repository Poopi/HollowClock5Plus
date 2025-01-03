#include "ClockWebServer.h"
#include "HollowClock.h"
#include "PreferencesManager.h"
#include "Zones.h"
#include "config.h"
#include <Arduino.h>

#include <WiFi.h>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#if DEBUG
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#else
#define TRACE(...)
#define ERROR(...)
#endif

static const String credits = R"(
    <div class="row"></div>
    <div class="row"></div>
    <div class="row">
        <label>Version: )" STRING_VERSION R"( ()" STRING_DATE R"()</label>
    </div>
    <div class="row">
        <label>©2025 HollowClock5Plus by <a href="https://github.com/Poopi">Poopi</a></label>
    </div>
    <div class="row">
        <label>©2024 Original hardware design by <a href="https://www.thingiverse.com/shiura">Shiura</a></label>
    </div>)";

ClockWebServer &ClockWebServer::getInstance() {
  static ClockWebServer instance;
  return instance;
}

void ClockWebServer::start() {
  webServer = new WebServer(WEBSERVER_PORT);
  setServerRouting();
  webServer->begin();
}

void ClockWebServer::setPreferenceHandle(Preferences *prefs) {
  this->prefs = prefs;
}

void ClockWebServer::handleClient() { webServer->handleClient(); }

void ClockWebServer::setServerRouting() {
  webServer->on(F("/"), HTTP_GET, std::bind(&ClockWebServer::handleRoot, this));
  webServer->on(F("/wifi.html"), HTTP_GET,
                std::bind(&ClockWebServer::handleWifi, this));
  webServer->on(F("/time.html"), HTTP_GET,
                std::bind(&ClockWebServer::handleTime, this));
  webServer->on(F("/advanced.html"), HTTP_GET,
                std::bind(&ClockWebServer::handleAdvanced, this));
  webServer->on(F("/position.html"), HTTP_GET,
                std::bind(&ClockWebServer::handlePosition, this));
  webServer->on(F("/error.html"), HTTP_GET,
                std::bind(&ClockWebServer::handleError, this));
  webServer->on(F("/styles.css"), HTTP_GET,
                std::bind(&ClockWebServer::handleStyles, this));
  /* API calls*/
  webServer->on(F("/zones"), HTTP_GET, zones_ListJson);
  webServer->on(F("/wifi"), HTTP_GET,
                std::bind(&ClockWebServer::handleWifiGet, this));
  webServer->on(F("/wifi"), HTTP_POST,
                std::bind(&ClockWebServer::handleWifiPost, this));
  webServer->on(F("/ssidlist"), HTTP_GET,
                std::bind(&ClockWebServer::handleSsidListGet, this));
  webServer->on(F("/time"), HTTP_GET,
                std::bind(&ClockWebServer::handleTimeGet, this));
  webServer->on(F("/time"), HTTP_POST,
                std::bind(&ClockWebServer::handleTimePost, this));
  webServer->on(F("/advanced"), HTTP_GET,
                std::bind(&ClockWebServer::handleAdvancedGet, this));
  webServer->on(F("/advanced"), HTTP_POST,
                std::bind(&ClockWebServer::handleAdvancedPost, this));
  webServer->on(F("/calibration"), HTTP_POST,
                std::bind(&ClockWebServer::handleCalibrationPost, this));
  webServer->on(F("/position"), HTTP_GET,
                std::bind(&ClockWebServer::handlePositionGet, this));
  webServer->on(F("/apply"), HTTP_POST,
                std::bind(&ClockWebServer::handleApplyPost, this));
  webServer->on(F("/reset"), HTTP_POST,
                std::bind(&ClockWebServer::handleResetPost, this));
}

void ClockWebServer::handleRoot() {
  static const String data = R"(
<!DOCTYPE html>
<script>
    function showAlert(text) {
        return confirm(text);
    }
</script>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Holow Clock 5 Plus</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" type="text/css" href="styles.css">
    <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgo=">
</head>
<body>
    <div class="row">
        <div class="cell">
            <h2>Hollow Clock Settings</h2>
        </div>
    </div>
    <div class="table-container">
        <div class="cell">
            <div class="row-button">
                <div class="separator"><button type="button" class="button-big" ; onclick="window.location.href = '/wifi.html'">Wifi
                        Settings</button></div>
                <div class="separator"><button type="button" class="button-big" ; onclick="window.location.href = '/time.html'">Time
                        Settings</button></div>

            </div>
        </div>
        <div class="cell">
            <div class="row-button">
                <div class="separator"><button type="button" class="button-big" ;
                        onclick="window.location.href = '/advanced.html'">Advanced Settings</button></div>
                <div class="separator"><button type="button" class="button-big" ;
                        onclick="window.location.href = '/position.html'">Position Calibration</button></div>
            </div>
        </div>
    </div>
    <div class="row">
        <form action="/apply" method="post" onsubmit='return showAlert("Are you sure you want to apply changes?")'>
            <button class="button" type="submit" value="Apply">Apply</button>
        </form>
    </div>
    <div class="row">
        <form action="/reset" method="post"
            onsubmit='return showAlert("Are you sure you want to reset settings to factory defaults?")'>
            <button class="button red-bg" type="submit">Factory reset</button>
        </form>
    </div>)" + credits + R"(
</body>
</html>
)";
  webServer->send(200, "text/html", data);
}

void ClockWebServer::handleStyles() {
  static const String data = R"(
.button,
.select,
.input,
.input-narrow,
.button-big{
    height: 30px;
    margin: 5px;
    border: 2px;
    border-style: solid;
    border-radius: 3px;
    color: #000000;
    border-color: #000000;
    background-color: #F8F8F8;
}

.button {
    width: 110px;
    background-color: #BBBBBB;
}

.red-bg {
    background-color: #FF0000;
}

.red-fg {
    color: #FF0000;
}

.checkbox {
    width: 25px;
    height: 25px;
}

.button-big {
    height: 70px;
    width: 120px;
    border-radius: 8px;
}

.input {
    width: 180px;
}

.input-narrow {
    width: 120px;
}

label {
    margin: 3px;
}

.separator {
    padding:3px;
}
.row,
.table-row {
    text-align: center;
}


.row-button {
    display: flex;
}

.table-container {
    display: table;
    margin: 0px auto;
}

.table-row {
    display: table-row;
}

.table-cell {
    display: table-cell;
    padding: 3px;
    vertical-align: middle;
}

.no-padding {
    padding: 0px;
}

.aleft {
    text-align: left;
}

.aright {
    text-align: right;
}

.disabled .readonly {
    background-color: #888888;
    color: #ffa0a0;
}

.disabled {
    display: none;
}

div,
button,
select,
label,
input[type="submit"] {
    font-family: Arial, sans-serif;
    font-size: 14px;
    padding: 4px;
}

h2 {
    font-size: 24px;
}

.error {
    font-size: 20px;
    font-weight: bold;
}

.tooltip {
    position: relative;
    font-weight: bold;
    vertical-align: middle;
}

.tooltip .tooltiptext {
    visibility: hidden;
    width: 120px;
    background-color: #555;
    color: #fff;
    text-align: center;
    padding: 5px;
    border-radius: 8px;

    position: absolute;
    z-index: 1;
    bottom: 100%;
    right: 0%;
    opacity: 0;
    transition: opacity 0.5s;
}

.tooltip .tooltiptext::after {
    content: "";
    position: absolute;
    top: 100%;
    left: 50%;
    margin-left: -5px;
    border-width: 10px;
    border-style: solid;
    border-color: #555 transparent transparent transparent;
}

.tooltip:hover .tooltiptext {
    visibility: visible;
    opacity: 1;
}
    )";
  webServer->send(200, "text/css", data);
}

void ClockWebServer::handleWifi() {
  static const String data = R"(
<!DOCTYPE html>
<script>
    let scanInterval;

    function scanWifi() {
        var scanButton = document.querySelector("button[onclick='scanWifi() ']");
        var dots = 0;
        scanButton.disabled = true;
        scanButton.textContent = 'Scanning';

        scanInterval = setInterval(() => {
            dots = (dots + 1) % 4;
            scanButton.textContent = 'Scanning' + '.'.repeat(dots);
        }, 500);
        
        fetch('/ssidlist')
            .then(response => response.json())
            .then(data => {
                clearInterval(scanInterval);
                scanButton.textContent = 'Scan';
                scanButton.disabled = false;

                var wifiList = document.getElementById('wifiList');
                wifiList.innerHTML = '';
                data.wifilist.forEach((network, index) => {
                    var option = document.createElement('option');
                    option.value = network.ssid;
                    option.textContent = `${network.ssid} (${network.signal_strength} dBm)`;
                    wifiList.appendChild(option);
                });
                if (data.wifilist.length > 0) {
                    wifiList.prepend(new Option('Select WiFi', '', true, true));
                    wifiList.options[0].disabled = true;
                }
            })
            .catch(error => {
                clearInterval(scanInterval);
                scanButton.textContent = 'Scan';
                scanButton.disabled = false;
                console.error('Error fetching WiFi list:', error);
            });
    }

    function updateSSID() {
        var wifiList = document.getElementById('wifiList');
        var ssidInput = document.getElementById('ssid');
        var selectedOption = wifiList.options[wifiList.selectedIndex].value;
        ssidInput.value = selectedOption;
    }

    document.addEventListener('DOMContentLoaded', (event) => {
        fetch('/wifi')
            .then(response => response.json())
            .then(data => {
                document.getElementById('ssid').value = data.ssid || '';
                document.getElementById('password').value = data.password || '';
            })
            .catch(error => console.error('Error fetching WiFi settings:', error));
    });
</script>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Holow Clock 5 Plus WiFi Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgo=">
    <link rel="stylesheet" type="text/css" href="styles.css">
</head>
<body>
    <div class="row">
        <div class="cell">
            <h2>Wifi Settings</h2>
        </div>
    </div>

    <form action="/wifi" method="post">
        <div class="table-container">
            <div class="table-row">
                <div class="table-cell aright">
                    <button class="button" type="button" onclick="scanWifi() ">Scan</button>
                </div>
                <div class="table-cell aleft">
                    <select id="wifiList" class="select" onchange="updateSSID() ">
                        <option value="">Not scanned</option>
                    </select>
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright">
                    <label for="ssid">Selected WiFi</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="text" id="ssid" name="ssid" value="">
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright">
                    <label for="password">Password</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="password" id="password" name="password">
                </div>
            </div>
        </div>
        <div class="row">
            <button type="submit" class="button" value="Save">Save</button>
        </div>
    </form>

    <div class="row">
        <div class="cell">
            <button type="submit" class="button" onclick='location.href="/"'>Back</button>
        </div>
    </div>
</body>
</html>
    )";
  webServer->send(200, "text/html", data);
}

void ClockWebServer::handleTime() {
  static const String data = R"(
<!DOCTYPE html>
<script>
    zones = [];

    function setTimezoneMode(manual) {
        if (manual) {
            document.getElementById('timezone_manual').classList.remove("disabled");
            document.getElementById('timezone_list').classList.add("disabled");
        } else {
            document.getElementById('timezone_manual').classList.add("disabled");
            document.getElementById('timezone_list').classList.remove("disabled")
        }
    }
    
    function updateTz() {
        document.getElementById('timezone_value').value = zones[document.getElementById('timezone_location').value];
    }

    document.addEventListener('DOMContentLoaded', (event) => {
        document.getElementById('tz_manual_en').addEventListener('change', function () {
            setTimezoneMode(this.checked);
        });
    });

    fetch('/zones')
        .then(response => response.json())
        .then(data => {
            zones = data;
            const timezoneSelect = document.getElementById('timezone_location');
            for (let zone in data) {
                const option = document.createElement('option');
                option.text = zone;
                option.value = zone;
                option.priv = data[zone];
                timezoneSelect.appendChild(option);
            };

            fetch('/time')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('ntp_server').value = data.ntp_server || '';
                    document.getElementById('timezone_location').value = data.timezone_location || '';
                    document.getElementById('timezone_value').value = data.timezone_value || '';
                    setTimezoneMode(data.tz_manual_en || false);
                    document.getElementById('tz_manual_en').checked = data.tz_manual_en || false;
                    document.getElementById('tz_manual').value = data.tz_manual || '';
                    updateTz();
                })
                .catch(error => console.error('Error fetching time settings:', error));
        })
        .catch(error => console.error('Error fetching timezones:', error));

</script>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Holow Clock 5 Plus Time Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgo=">
    <link rel="stylesheet" type="text/css" href="styles.css">
</head>
<body>
    <div class="row">
        <div class="cell">
            <h2>Time Settings</h2>
        </div>
    </div>
    <form id="time_form" action="/time" method="post">
        <div class="table-container">
            <div class="table-row">
                <div class="table-cell aright tooltip">
                    <span class="tooltiptext">Sets the timezone to manual value</span>
                    <label for="tz_manual_en">Manual Timezone </label>
                </div>
                <div class="table-cell aleft">
                    <input class="checkbox" type="checkbox" id="tz_manual_en" name="tz_manual_en">
                </div>
            </div>

            <div class="table-row" id="timezone_manual">
                <div class="table-cell aright">
                    <label for="tz_manual">Time offset in minutes</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="text" id="tz_manual" name="tz_manual" value="">
                </div>
            </div>

            <div class="table-row" id="timezone_list">
                <div class="table-cell aright">
                    <label for="timezone_location">Time zone</label>
                </div>
                <div class="table-cell aleft">
                    <select class="select" type="text" id="timezone_location" name="timezone_location" value="" onchange="updateTz() ">
                    </select>
                </div>
                <div class="table-cell">
                    <input class="input" type="text" id="timezone_value" name="timezone_value" value="" readonly hidden>
                </div>
            </div>

            <div class="table-row">
                <div class="table-cell aright">
                    <label for="ntp_server">NTP Server</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="text" id="ntp_server" name="ntp_server" value="">
                </div>
            </div>
        </div>
        <div class="row">
            <button class="button" type="submit" value="Save" onclick="updateTz() ">Save</button>
        </div>
    </form>

    <div class="row">
        <div class="cell">
            <button type="submit" class="button" onclick='location.href="/"'>Back</button>
        </div>
    </div>
</body>
</html>
  )";
  webServer->send(200, "text/html", data);
}

void ClockWebServer::handleAdvanced() {
  static const String data = R"(
<!DOCTYPE html>
<script>
    document.addEventListener('DOMContentLoaded', (event) => {
        fetch('/advanced')
            .then(response => response.json())
            .then(data => {
                document.getElementById('host_name').value = data.host_name || 'HollowClock';
                document.getElementById('host_ip').value = data.host_ip || '192.168.100.1';
                document.getElementById('flip_rotation').checked = data.flip_rotation || false;
                document.getElementById('allow_backward').checked = data.allow_backward || false;
                document.getElementById('steps_per_minute').value = data.steps_per_minute || 256;
                document.getElementById('delay_time').value = data.delay_time || 2;
            })
            .catch(error => console.error('Error fetching advanced settings:', error));
    });
</script>

<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Holow Clock 5 Plus Advanced Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgo=">
    <link rel="stylesheet" type="text/css" href="styles.css">
</head>
<body>
    <div class="row">
        <div class="cell">
            <h2>Advanced Settings</h2>
        </div>
    </div>
    <form action="/advanced" method="post">
        <div class="table-container">
            <div class="table-row">
                <div class="table-cell aright">
                    <label for="host_name"> Host Name</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="text" id="host_name" name="host_name" value=""><br>
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright tooltip">
                    <span class="tooltiptext">This is the IP clock will use when act as access point</span>
                    <label for="host_ip"> Host IP</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="text" id="host_ip" name="host_ip" value="" inputmode="numeric" pattern="^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)\.?\b){4}$" autocomplete="off" maxlength="15" placeholder="xxx.xxx.xxx.xxx">
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright tooltip">
                    <span class="tooltiptext">Check if your motor rotate to the opposite direction</span>
                    <label for="flip_rotation"> Flip Rotation</label>
                </div>
                <div class="table-cell aleft">
                    <input class="checkbox" type="checkbox" id="flip_rotation" name="flip_rotation" value="on">
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright tooltip">
                    <span class="tooltiptext">Allow backward move - speeds up calibration - don't use with ratched installed</span>
                    <label for="flip_rotation"> Allow backward</label>
                </div>
                <div class="table-cell aleft">
                    <input class="checkbox" type="checkbox" id="allow_backward" name="allow_backward" value="on">
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright tooltip">
                    <span class="tooltiptext">Adjust value if the clock is too fast or too slow. Default 256</span>
                    <label for="steps_per_minute">Number of step per minute</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="number" id="steps_per_minute" name="steps_per_minute" value="256" min="1" max="1024" step="1">
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright tooltip">
                    <span class="tooltiptext">Wait for a single step of stepper. Should be bigger than 1. Default value is 2</span>
                    <label for="delay_time">Delay time</label>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="number" id="delay_time" name="delay_time" value="2" min="2" max="100" step="1">

                </div>
            </div>
        </div>
        <div class="row">
            <button class="button" type="submit" value="Save">Save</button>
        </div>
    </form>
    <div class="row">
        <div class="cell">
            <button class="button" type="submit" onclick='location.href="/"'>Back</button>
        </div>
    </div>
</body>
</html>
    )";
  webServer->send(200, "text/html", data);
}

void ClockWebServer::handlePosition() {
  static const String data = R"(
<!DOCTYPE html>
<script>
    document.addEventListener("DOMContentLoaded", function() {
        function fetchPositionData() {
            fetch('/position')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('local_time_value').textContent = data.local_time;
                    document.getElementById('hands_position_value').textContent = data.hands_position;
                })
                .catch(error => console.error('Error fetching position data:', error));
        }

        fetchPositionData();
        setInterval(fetchPositionData, 2000);
    });
</script>

<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Holow Clock 5 Plus Position Calibration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgo=">
    <link rel="stylesheet" type="text/css" href="styles.css">
</head>
<body>
    <div class="row">
        <div class="cell">
            <h2>Calibration Settings</h2>
        </div>
    </div>
    <div class="table-container">
        <div class="table-row">
            <div class="table-cell aright">
                <label id="local_time">Local Time</label>
            </div>
            <div class="table-cell aleft">
                <label id="local_time_value">00:00</label>
            </div>
        </div>
        <div class="table-row">
            <div class="table-cell aright">
                <label id="hands_position">Assumed position of hands</label>
            </div>
            <div class="table-cell aleft">
                <label id="hands_position_value">00:00</label>
            </div>
        </div>    </div>
    <form action="/calibration" method="post">
        <div class="table-container">
            <div class="table-row">
                <div class="table-cell aright">
                    <button type="submit" class="button" name="start" value="start">Start</button>
                </div>
                <div class="table-cell aleft">
                    <button type="submit" class="button" name="stop" value="stop">Stop</button>
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright">
                    <button type="submit" class="button" name="move" value="move">Move</button>
                </div>
                <div class="table-cell aleft">
                    <input class="input" type="number" id="steps" name="steps" value="" min="-184320" max="184320" step="256" placeholder="Number of steps"><br>
                </div>
            </div>
            <div class="table-row">
                <div class="table-cell aright">
                    <button type="submit" class="button" name="set" value="set">Set Hands</button>
                </div>
                <div class="table-cell">
                    <div class="table-cell aleft no-padding">
                        <div class="table-row no-padding">
                            <div class="table-cell aleft no-padding">
                                <input class="input-narrow" type="number" id="hour_hand" name="hour_hand" placeholder="Hours pos(0-11) " value="" min="0" max="11" step="1"><br>
                            </div>
                            <div class="table-cell aleft no-padding">
                                <input class="input-narrow" type="number" id="minute_hand" name="minute_hand" placeholder="Min pos(0-59) " value="" min="0" max="59" step="1"><br>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </form>
    <div class="row">
        <div class="cell">
            <button type="submit" class="button" onclick='location.href="/"'>Back</button>
        </div>
    </div>
</body>
</html>
    )";
  webServer->send(200, "text/html", data);
}

void ClockWebServer::sendError(const String &message) {
  lastError = message;
  webServer->sendHeader("Location", String("/error.html"), true);
  webServer->send(302, "text/plain", "");
}

void ClockWebServer::handleError() {
  static const String data = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>ERROR</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" type="text/css" href="styles.css">
    <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgo=">
</head>

<body>
    <div class="row">
        <div class="cell">
            <div class="red-fg error">)" +
                             lastError +
                             R"(</div>
        </div>
    </div>
    <div class="row">
        <div class="cell">
            <button type="submit" class="button" onclick="javascript:history.back() ">Back</button>
        </div>
    </div>
</body>
</html>
    )";
  webServer->send(400, "text/html", data);
}

void ClockWebServer::handleWifiGet() {
  PreferencesManager &pm = PreferencesManager::getInstance();

  String ssid = pm.getSSID();
  String passwd = pm.getPassword();
  String data = R"(
    {
    "ssid": ")" +
                ssid + R"(",
    "password": ")" +
                passwd + R"("
    }
    )";
  webServer->send(200, "application/json", data);
}

void ClockWebServer::handleWifiPost() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  String ssid = webServer->arg("ssid");
  String passwd = webServer->arg("password");
  if (pm.setSSID(ssid) != PREF_OK) {
    sendError("Failed to set SSID");
    return;
  }
  if (pm.setPassword(passwd) != PREF_OK) {
    sendError("Failed to set Password");
    return;
  }
  webServer->sendHeader("Location", String("/"), true);
  webServer->send(302, "text/plain", "");

  TRACE("SSID: %s, Password: %s\n", ssid.c_str(), passwd.c_str());
}

void ClockWebServer::handleSsidListGet() {
  int n = WiFi.scanNetworks();

  std::vector<std::pair<String, int>> wifiList;
  for (int i = 0; i < n; ++i) {
    wifiList.push_back({WiFi.SSID(i), WiFi.RSSI(i)});
  }

  // Sort by signal strength (RSSI) in descending order
  std::sort(wifiList.begin(), wifiList.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  // Remove duplicates
  std::set<String> seenSSIDs;
  wifiList.erase(std::remove_if(wifiList.begin(), wifiList.end(),
                                [&seenSSIDs](const auto &item) {
                                  if (seenSSIDs.find(item.first) !=
                                      seenSSIDs.end()) {
                                    return true;
                                  } else {
                                    seenSSIDs.insert(item.first);
                                    return false;
                                  }
                                }),
                 wifiList.end());

  String data = R"({"wifilist":[)";
  for (size_t i = 0; i < wifiList.size(); ++i) {
    if (i > 0)
      data += ",";
    data += R"({"ssid":")" + wifiList[i].first + R"(","signal_strength":)" +
            String(wifiList[i].second) + R"(})";
  }
  data += R"(]})";
  webServer->send(200, "application/json", data);
}

void ClockWebServer::handleTimeGet() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  String ntpServer = pm.getNTPServer();
  String timezone = pm.getTimeZone();
  String timezone_location = pm.getTimeZoneLocation();
  bool manual = pm.getManualTimezone();
  int manualValue = pm.getManualTimezoneValue();
  String data = R"(
    {
    "ntp_server": ")" +
                ntpServer + R"(",
    "timezone_location": ")" +
                timezone_location + R"(",
    "timezone_value": ")" +
                timezone + R"(",
    "tz_manual_en": )" +
                (manual ? "true" : "false") + R"(,
    "tz_manual": ")" +
                String(manualValue) + R"("
    }
    )";
  webServer->send(200, "application/json", data);
}

void ClockWebServer::handleTimePost() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  String ntpServer = webServer->arg("ntp_server");
  String timezone = webServer->hasArg("timezone_value")
                        ? webServer->arg("timezone_value")
                        : "";
  String timezone_location = webServer->hasArg("timezone_location")
                                 ? webServer->arg("timezone_location")
                                 : "";

  bool manual = webServer->hasArg("tz_manual_en") &&
                webServer->arg("tz_manual_en") == "on";
  int manualValue =
      webServer->hasArg("tz_manual") ? webServer->arg("tz_manual").toInt() : 0;

  if (pm.setNTPServer(ntpServer) != PREF_OK) {
    sendError("Invalid NTP Server");
    return;
  }

  if (pm.setManualTimezone(manual) != PREF_OK) {
    sendError("Failed to set Manual Timezone");
    return;
  }

  if (manual) {
    if (pm.setManualTimezoneValue(manualValue) != PREF_OK) {
      sendError("Failed to set Manual Timezone Value");
      return;
    }
  } else {
    if (pm.setTimeZone(timezone) != PREF_OK) {
      sendError("Failed to set TimeZone");
      return;
    }
    if (pm.setTimeZoneLocation(timezone_location) != PREF_OK) {
      sendError("Failed to set TimeZone Location");
      return;
    }
  }

  webServer->sendHeader("Location", String("/"), true);
  webServer->send(302, "text/plain", "");

  TRACE("NTPServer: %s, TimeZone: %s, TimezoneLocation:%s Manual: %d, "
        "ManualValue: %d\n",
        ntpServer.c_str(), timezone.c_str(), timezone_location.c_str(), manual,
        manualValue);
}

void ClockWebServer::handleAdvancedGet() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  String hostName = pm.getHostName();
  String hostIP = pm.getServerIP();
  bool flipRotation = pm.getFlipRotation();
  bool allowBackward = pm.getAllowBackward();
  uint32_t stepsPerMinute = pm.getStepsPerMinute();
  uint8_t delayTime = pm.getDelayTime();
  String data = R"(
    {
    "host_name": ")" +
                hostName + R"(",
    "host_ip": ")" +
                hostIP + R"(",
    "flip_rotation": )" +
                (flipRotation ? "true" : "false") + R"(,
    "allow_backward": )" +
                (allowBackward ? "true" : "false") + R"(,
    "steps_per_minute": )" +
                String(stepsPerMinute) + R"(,
    "delay_time": )" +
                String(delayTime) + R"(
    }
    )";
  webServer->send(200, "application/json", data);
}

void ClockWebServer::handleAdvancedPost() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  String hostName =
      webServer->hasArg("host_name") ? webServer->arg("host_name") : "";
  String hostIP = webServer->hasArg("host_ip") ? webServer->arg("host_ip") : "";
  bool flipRotation = webServer->hasArg("flip_rotation") &&
                      webServer->arg("flip_rotation") == "on";
  bool allowBackward = webServer->hasArg("allow_backward") &&
                       webServer->arg("allow_backward") == "on";
  uint32_t stepsPerMinute = webServer->arg("steps_per_minute").toInt();
  uint8_t delayTime = webServer->arg("delay_time").toInt();
  if (pm.setHostName(hostName) != PREF_OK) {
    sendError("Failed to set Host Name");
    return;
  }
  if (pm.setServerIP(hostIP) != PREF_OK) {
    sendError("Failed to set Host IP");
    return;
  }
  if (pm.setFlipRotation(flipRotation) != PREF_OK) {
    sendError("Failed to set Flip Rotation");
    return;
  }
  if (pm.setAllowBackward(allowBackward) != PREF_OK) {
    sendError("Failed to set Allow Backward");
    return;
  }
  if (pm.setStepsPerMinute(stepsPerMinute) != PREF_OK) {
    sendError("Failed to set Steps Per Minute");
    return;
  }
  if (pm.setDelayTime(delayTime) != PREF_OK) {
    sendError("Failed to set Delay Time");
    return;
  }
  webServer->sendHeader("Location", String("/"), true);
  webServer->send(302, "text/plain", "");
  TRACE("Set: HostName: %s, HostIP: %s, FlipRotation: %d, StepsPerMinute: %d, "
        "DelayTime: %d\n",
        hostName.c_str(), hostIP.c_str(), flipRotation, stepsPerMinute,
        delayTime);
}

void ClockWebServer::handleCalibrationPost() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  HollowClock &hclock = HollowClock::getInstance();

  bool clock_start =
    webServer->hasArg("start") && webServer->arg("start") == "start" ? true
                                                                       : false;
  bool clock_stop =
      webServer->hasArg("stop") && webServer->arg("stop") == "stop" ? true
                                                                    : false;
  bool clock_move =
      webServer->hasArg("move") && webServer->arg("move") == "move" ? true
                                                                    : false;
  bool clock_set =
      webServer->hasArg("set") && webServer->arg("set") == "set" ? true : false;

  if (clock_start) {
    if (hclock.moveStart() == HCLOCK_OK) {
      webServer->sendHeader("Location", String("/position.html"), true);
      webServer->send(302, "text/plain", "");
    } else {
      sendError("Failed to send command - queue full");
    }
  } else if (clock_stop) {
    if (hclock.moveStop() == HCLOCK_OK) {
      webServer->sendHeader("Location", String("/position.html"), true);
      webServer->send(302, "text/plain", "");
    } else {
      sendError("Failed to send command - queue full");
    }
  } else if (clock_move) {
    int steps = webServer->arg("steps").toInt();
    if (steps < -184320 || steps > 184320) {
      sendError("Invalid number of steps");
    } else {
      if (hclock.moveSteps(steps) == HCLOCK_OK) {
        TRACE("Moved clock by %d steps\n", steps);
        webServer->sendHeader("Location", String("/position.html"), true);
        webServer->send(302, "text/plain", "");
      } else {
        sendError("Failed to send command - queue full");
      }
    }
  } else if (clock_set) {
    int hourHand = webServer->arg("hour_hand").toInt();
    int minuteHand = webServer->arg("minute_hand").toInt(); /* code */
    if (hourHand < 0 || hourHand > 11 || minuteHand < 0 || minuteHand > 59) {
      String errorMsg = "Invalid position: HourHand: " + String(hourHand) +
                        ", MinuteHand: " + String(minuteHand);
      TRACE("%s\n", errorMsg.c_str());
      sendError(errorMsg);
    } else {
      if (hclock.updateClockPosition(hourHand, minuteHand) == HCLOCK_OK) {
        TRACE("HourHand: %d, MinuteHand: %d\n", hourHand, minuteHand);
        webServer->sendHeader("Location", String("/position.html"), true);
        webServer->send(302, "text/plain", "");
      } else {
        sendError("Failed to send command - queue full");
      }
    }
  }
}

void ClockWebServer::handlePositionGet() {
  String data = R"( 
    {
    "local_time": ")" +
                HollowClock::getInstance().getLocalTime() + R"(",
    "hands_position": ")" +
                HollowClock::getInstance().getHandsPosition() + R"("
    }
    )";
  webServer->send(200, "application/json", data);
}
void ClockWebServer::handleApplyPost() {
  HollowClock &hclock = HollowClock::getInstance();

  webServer->sendHeader("Location", String("/"), true);
  webServer->send(302, "text/plain", "");

  if (hclock.isCalibrated()) {
    hclock.saveClockPosition();
  }
  delay(1000);
  ERROR("Rebooting....\n");
#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
  esp_deep_sleep_start();
#else
  ESP.restart();
#endif
}

void ClockWebServer::handleResetPost() {
  PreferencesManager &pm = PreferencesManager::getInstance();
  webServer->sendHeader("Location", String("/"), true);
  webServer->send(302, "text/plain", "");
  pm.eraseAll();
  delay(500);
  ERROR("Rebooting....\n");
#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
  esp_deep_sleep_start();
#else
  ESP.restart();
#endif
}

void ClockWebServer::send(int code, const char *content_type,
                          const String &data) {
  webServer->send(code, content_type, data);
}