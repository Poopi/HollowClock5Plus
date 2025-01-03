#ifndef _HOLLOW_CLOCK_H
#define _HOLLOW_CLOCK_H

#include <Arduino.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

typedef enum {
  HCLOCK_OK = 0,
  HCLOCK_ERROR = -1,
} hclock_result_t;

class HollowClock {

public:
  static HollowClock &getInstance();
  HollowClock(const HollowClock &) = delete;
  HollowClock &operator=(const HollowClock &) = delete;

  void rotate(int steps, int delaytime);
  void setDirection(bool direction);
  void setAllowBackwardMovement(bool allow);
  void saveClockPosition(void);
  bool isCalibrated(void);
  bool isPositioning(void);
  void getClockPosition(uint8_t &hours, uint8_t &minutes);
  String getLocalTime(void);
  String getHandsPosition(void);

  hclock_result_t moveStart(void);
  hclock_result_t moveStop(void);
  hclock_result_t moveSteps(int steps);
  hclock_result_t updateClockPosition(uint8_t hours, uint8_t minutes);

  void start(void);

private:
  HollowClock();
  ~HollowClock() = default;

  uint32_t makeCommand(uint8_t cmd, uint8_t val1, uint8_t val2);
  uint32_t makeCommand(uint8_t cmd, int val);
  bool addToQueue(uint32_t value);

  uint8_t getCommand(uint32_t value);
  void getParams(uint32_t value, uint8_t &par1, uint8_t &par2);
  void getParams(uint32_t value, int &par);
  bool getFromQueue(uint32_t &value);

  bool flip_rotation;
  bool allow_backward_movement;
  bool positioning = false;
  uint32_t steps_per_minute;
  uint8_t delay_time;
  int phase = 0;
  int ports[2][4];
  std::atomic<uint32_t> clock_position;
  uint32_t max_clock_position;

  // sequence of stepper motor control
  const int seq[4][4] = {{LOW, LOW, HIGH, LOW},
                         {LOW, LOW, LOW, HIGH},
                         {HIGH, LOW, LOW, LOW},
                         {LOW, HIGH, LOW, LOW}};

  void threadFunction(void);
  std::thread clockThread;

  static const int QUEUE_SIZE = 3;
  std::queue<uint32_t> commandQueue;
  std::mutex threadMutex;
  std::condition_variable queueCondition;
};

#endif
