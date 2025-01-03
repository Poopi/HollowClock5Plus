#ifndef _CONFIG_H
#define _CONFIG_H

#include <Arduino.h>

#define DEBUG 0
#define USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK 0
#define MAX_FAST_MOVMENT_STEPS 1000

// Ports used for the stepper motor
#define CONFIG_MOTOR_PORTS {1, 2, 21, 22}
#define SERIAL_BAUD_RATE 115200
#define WEBSERVER_PORT 80
#define DNS_PORT 53
#define DEFAULT_NTP_SERVER "pool.ntp.org"
#define DEFAULT_TIMEZONE_LOCATION "Etc/GMT"
#define DEFAULT_TIMEZONE "GMT0"
#define DEFAULT_AP_NAME_PREFIX "HOLLOW5P-"

#define STRING_VERSION "0.9.0"
#define STRING_DATE "7 January 2025"

#endif