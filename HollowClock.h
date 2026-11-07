#ifndef _HOLLOW_CLOCK_H
#define _HOLLOW_CLOCK_H

#include <Arduino.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "PositionLedger.h"

typedef enum {
  HCLOCK_OK = 0,
  HCLOCK_ERROR = -1,
} hclock_result_t;

class HollowClock {

public:
  static HollowClock &getInstance();
  HollowClock(const HollowClock &) = delete;
  HollowClock &operator=(const HollowClock &) = delete;

  void setDirection(bool direction);
  void setAllowBackwardMovement(bool allow);
  void saveClockPosition(void);
  bool isCalibrated(void);
  bool isPositioning(void);
  hclock_result_t getClockPosition(uint8_t &hours, uint8_t &minutes);
  String getLocalTime(void);
  String getLastSyncedTime(void);
  void setLastSyncedTime(String time);
  String getHandsPosition(void);

  hclock_result_t moveStart(void);
  hclock_result_t moveStop(void);
  hclock_result_t moveSteps(int steps);
  hclock_result_t updateClockPosition(uint8_t hours, uint8_t minutes);

  void start(void);

private:
  HollowClock();
  ~HollowClock() = default;

  void moveRecorded(int steps, int delaytime);
  int calculateTimeDiff(int local_clock_position, int current_position,
                        bool &direction_forward);
  void playChime(int current_time);

  uint32_t makeCommand(uint8_t cmd, uint8_t val1, uint8_t val2);
  uint32_t makeCommand(uint8_t cmd, int val);
  bool addToQueue(uint32_t value);

  uint8_t getCommand(uint32_t value);
  void getParams(uint32_t value, uint8_t &par1, uint8_t &par2);
  void getParams(uint32_t value, int &par);
  bool getFromQueue(uint32_t &value);

  String last_synced_time;
  bool flip_rotation;
  bool allow_backward_movement;
  bool play_chime;
  bool started;
  bool positioning;
  uint32_t steps_per_minute;
  uint8_t delay_time;
  PositionLedger ledger;
  int max_clock_position;

  void threadFunction(void);
  std::thread clockThread;

  static const int QUEUE_SIZE = 3;
  std::queue<uint32_t> commandQueue;
  std::mutex threadMutex;
  std::condition_variable queueCondition;
};

#endif
