#include "HollowClock.h"
#include "PreferencesManager.h"
#include "config.h"
#include <thread>
#include <time.h>

#if DEBUG
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#else
#define TRACE(...)
#define ERROR(...)
#endif

#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
RTC_DATA_ATTR uint32_t unresettable_var;
#endif

enum {
  CMD_START = 1,          // start movement
  CMD_STOP = 2,           // stop movement
  CMD_STEP = 3,           // move steps
  CMD_UPDATE_POSITION = 4 // set hands at specific position
};

HollowClock &HollowClock::getInstance() {
  static HollowClock instance;
  return instance;
}

// original function from shiura modified by me
void HollowClock::rotate(int steps, int delaytime) {
  int stepIndex, phaseIndex;
  int delta = (steps > 0) ? 1 : 3;
  int dt;

  if (delaytime > 0) {
    dt = delaytime * 3;
  } else {
    delaytime = 2;
    dt = 2 * delaytime;
  }
  int idx = flip_rotation ? 1 : 0;

  int abs_steps = (steps > 0) ? steps : -steps;
  for (phaseIndex = 0; phaseIndex < abs_steps; phaseIndex++) {
    phase = (phase + delta) % 4;
    for (stepIndex = 0; stepIndex < 4; stepIndex++) {
      digitalWrite(ports[idx][stepIndex], seq[phase][stepIndex]);
    }

    delay(dt);
    if (dt > delaytime)
      dt--;
  }
  // power cut
  for (stepIndex = 0; stepIndex < 4; stepIndex++) {
    digitalWrite(ports[idx][stepIndex], LOW);
  }

  // Adjust current position
  if (clock_position != PreferencesManager::INVALID_CLOCK_POSITION)
    clock_position = (steps > 0)
                         ? (clock_position + steps) % max_clock_position
                         : (clock_position + max_clock_position + steps) %
                               max_clock_position;
  else
    clock_position = PreferencesManager::INVALID_CLOCK_POSITION;
}

void HollowClock::saveClockPosition(void) {
  PreferencesManager &pm = PreferencesManager::getInstance();
  pm.setClock(clock_position);
}

void HollowClock::setDirection(bool direction) { flip_rotation = direction; }

void HollowClock::setAllowBackwardMovement(bool allow) {
  allow_backward_movement = allow;
}

void HollowClock::threadFunction(void) {
  bool clock_moving = true;
  while (true) {
    struct tm timeinfo;

    // Check for any new command
    uint32_t value;
    if (getFromQueue(value)) {
      uint8_t command = getCommand(value);
      TRACE("Command received: %d\n", command);
      switch (command) {
      case CMD_START:
        clock_moving = true;
        break;
      case CMD_STOP:
        clock_moving = false;
        break;
      case CMD_STEP:
        int steps;
        getParams(value, steps);
        TRACE("CMD_STEP: %d\n", steps);
        if (steps > 0) {
          while (steps > 0) {
            int step = (steps > MAX_FAST_MOVMENT_STEPS) ? MAX_FAST_MOVMENT_STEPS
                                                        : steps;
            rotate(step, delay_time);
            steps -= step;
          }
        } else if (allow_backward_movement) {
          while (steps < 0) {
            int step = (steps < -MAX_FAST_MOVMENT_STEPS)
                           ? -MAX_FAST_MOVMENT_STEPS
                           : steps;
            rotate(step, delay_time);
            steps -= step;
          }
        }
        break;
      case CMD_UPDATE_POSITION:
        uint8_t hours, minutes;
        getParams(value, hours, minutes);
        clock_position = (hours * 60 + minutes) * steps_per_minute;
        saveClockPosition();
        break;
      }
      continue;
    }

    if (clock_moving) {
      if (!::getLocalTime(&timeinfo)) {
        TRACE("Failed to obtain time");
        delay(5000);
        continue;
      }

      int hour = timeinfo.tm_hour % 12;
      int minute = timeinfo.tm_min;
      int current_position = (hour * 60 + minute) * steps_per_minute;

      if (clock_position == PreferencesManager::INVALID_CLOCK_POSITION) {
        // We don't know the current position of the clock so just tick
        rotate(steps_per_minute / 16, delay_time);
        delay(60000 / 16);
      } else {
        if (current_position != clock_position) {
          int local_clock_position = (int)clock_position;
          bool direction_forward = true;
          int time_diff = 0;
          TRACE("Time diff: %d %d\n",
                abs(local_clock_position - current_position),
                (current_position + max_clock_position - clock_position) %
                    max_clock_position);
          if (allow_backward_movement) {
            if (abs(local_clock_position - current_position) <
                ((current_position + max_clock_position - clock_position) %
                 max_clock_position)) {
              direction_forward = false;
              time_diff = abs(local_clock_position - current_position);
            }
          }
          if (direction_forward) {
            time_diff =
                (current_position + max_clock_position - local_clock_position) %
                max_clock_position;
          }
          if (time_diff > steps_per_minute) {
            positioning = true;
            TRACE("Positioning: current position: %d, Clock position: %d - "
                  "%stime_diff(sec):%d\n",
                  current_position, local_clock_position,
                  direction_forward ? "" : "-",
                  time_diff * 60 / steps_per_minute);

            time_diff = (time_diff > MAX_FAST_MOVMENT_STEPS)
                            ? MAX_FAST_MOVMENT_STEPS
                            : time_diff;
            if (time_diff <= MAX_FAST_MOVMENT_STEPS) {
              saveClockPosition();
            }
            rotate(direction_forward ? time_diff : -time_diff,
                   -1); // move fast to the current position
            positioning = false;
            continue;
          } else {
            int32_t local_clock_position = clock_position;
            TRACE("Current position: %d, Clock position: %d - "
                  "time_diff(sec):%d\n",
                  current_position, local_clock_position,
                  time_diff * 60 / steps_per_minute);
            rotate(time_diff, delay_time);
          }
        }
      }
    }
    delay(1000);
  }
}

bool HollowClock::isCalibrated(void) {
  return clock_position != PreferencesManager::INVALID_CLOCK_POSITION;
}

bool HollowClock::isPositioning(void) { return positioning; }

void HollowClock::getClockPosition(uint8_t &hours, uint8_t &minutes) {
  int position = clock_position;
  hours = position / (60 * steps_per_minute);
  minutes = (position % (60 * steps_per_minute)) / steps_per_minute;
}
hclock_result_t HollowClock::moveStart(void) {
  int value = makeCommand(CMD_START, 0);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}
hclock_result_t HollowClock::moveStop(void) {
  int value = makeCommand(CMD_STOP, 0);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}

hclock_result_t HollowClock::moveSteps(int steps) {
  int value = makeCommand(CMD_STEP, steps);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}

hclock_result_t HollowClock::updateClockPosition(uint8_t hours,
                                                 uint8_t minutes) {
  int value = makeCommand(CMD_UPDATE_POSITION, hours, minutes);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}

String HollowClock::getLocalTime(void) {
  struct tm timeinfo;
  if (!::getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }
  char time_str[6];
  strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
  return String(time_str);
}

String HollowClock::getHandsPosition(void) {
  uint8_t hours, minutes;
  getClockPosition(hours, minutes);
  char time_str[6];
  snprintf(time_str, sizeof(time_str), "%2d:%02d", hours, minutes);
  return String(time_str);
}

void HollowClock::start(void) {
  clockThread = std::thread(std::bind(&HollowClock::threadFunction, this));
  clockThread.detach();
}

// Function to add a command to the queue
bool HollowClock::addToQueue(uint32_t value) {
  TRACE("Adding command %X to queue\n", value);
  std::lock_guard<std::mutex> lock(threadMutex);
  bool result = false;
  if (commandQueue.size() < QUEUE_SIZE) {
    commandQueue.push(value);
    queueCondition.notify_one();
    result = true;
  }
  return result;
}

// Function to get a command from the queue non-blocking
bool HollowClock::getFromQueue(uint32_t &value) {
  std::lock_guard<std::mutex> lock(threadMutex);
  if (commandQueue.empty()) {
    return false; // Indicate that the queue is empty
  }
  value = commandQueue.front();
  TRACE("Getting command %X from queue\n", value);
  commandQueue.pop();
  return true; // Successfully retrieved a command
}

uint32_t HollowClock::makeCommand(uint8_t cmd, uint8_t val1, uint8_t val2) {
  return (cmd << 24) | (val1 << 16) | (val2 << 8);
}

uint32_t HollowClock::makeCommand(uint8_t cmd, int val) {
  int sign = (val < 0) ? 1 : 0;
  val = abs(val);
  return (cmd << 24) | (sign << 23) | (val & 0x7FFFFF);
}

uint8_t HollowClock::getCommand(uint32_t value) { return value >> 24; }

void HollowClock::getParams(uint32_t value, uint8_t &par1, uint8_t &par2) {
  par1 = (value >> 16) & 0xFF;
  par2 = (value >> 8) & 0xFF;
}

void HollowClock::getParams(uint32_t value, int &par) {
  par = (value & 0x7FFFFF) * ((value >> 23) & 0x1 ? -1 : 1);
}

HollowClock::HollowClock() {
  PreferencesManager &pm = PreferencesManager::getInstance();

  int motor_ports[4] = CONFIG_MOTOR_PORTS;

  ports[0][0] = motor_ports[0];
  ports[0][1] = motor_ports[1];
  ports[0][2] = motor_ports[2];
  ports[0][3] = motor_ports[3];
  ports[1][0] = motor_ports[3];
  ports[1][1] = motor_ports[2];
  ports[1][2] = motor_ports[1];
  ports[1][3] = motor_ports[0];

  pinMode(motor_ports[0], OUTPUT);
  pinMode(motor_ports[1], OUTPUT);
  pinMode(motor_ports[2], OUTPUT);
  pinMode(motor_ports[3], OUTPUT);

  flip_rotation = pm.getFlipRotation();
  allow_backward_movement = pm.getAllowBackward();
  steps_per_minute = pm.getStepsPerMinute();
  delay_time = pm.getDelayTime();
  clock_position = pm.getClockPosition();

  phase = 0;
  max_clock_position = 12 * 60 * steps_per_minute;

// Test
#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
  TRACE("Unresettable var: %d\n", unresettable_var);
  unresettable_var++;
#endif
}
