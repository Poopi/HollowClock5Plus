#ifndef _POSITION_LEDGER_H
#define _POSITION_LEDGER_H

#include <Arduino.h>
#include <atomic>

// Durable record of where the hands are.
//
// Every motion of the motor is bracketed by begin() and commit(). The record
// in NVS is written at both edges, so after a power loss the clock either
// knows the hand position exactly, or knows that a motion was cut short and
// says so (suspect()). The silent error is zero; the reported error is at
// most one motion, and a motion is capped at max_steps_per_move.
//
//   Uncalibrated --calibrate--> Settled
//   Settled      --begin-----> Moving
//   Moving       --commit----> Settled
//   any          --calibrate--> Settled (clears suspect)
//   power loss in Moving, then load(): Settled at the pre-motion position,
//                                      suspect = true
//
// NVS word layout (key "ClockPos", unchanged from the original firmware):
//   0xFFFFFFFF            uncalibrated (INVALID_CLOCK_POSITION)
//   bit 31  FLAG_MOVING   begin() wrote the record, commit() has not
//   bit 30  FLAG_SUSPECT  a motion was cut by power loss since calibrate()
//   bits 0-23             hand position in motor steps, < 12*60*steps_per_minute
// A record written by the original firmware has both flags clear and reads
// back as Settled and trusted.
class PositionLedger {
public:
  enum class State : uint8_t { Uncalibrated, Settled, Moving };

  // Read the record from NVS. Never writes.
  void load(uint32_t max_position, uint32_t max_steps_per_move);

  State state() const { return state_.load(); }
  bool calibrated() const { return state_.load() != State::Uncalibrated; }
  bool suspect() const { return suspect_.load(); }
  // Hand position in motor steps. Meaningful only when calibrated().
  uint32_t position() const { return position_.load(); }

  // Any state -> Settled, trusted. One NVS write.
  void calibrate(uint32_t position);
  // Settled -> Moving. One NVS write. Returns false, and writes nothing, when
  // the state is not Settled, steps is 0, or |steps| > max_steps_per_move.
  bool begin(int32_t steps);
  // Moving -> Settled, position advanced by the steps given to begin().
  // One NVS write. Does nothing when not Moving.
  void commit();

private:
  static const uint32_t FLAG_MOVING = 0x80000000u;
  static const uint32_t FLAG_SUSPECT = 0x40000000u;
  static const uint32_t POSITION_MASK = 0x00FFFFFFu;

  uint32_t wrap(int64_t position) const;
  void store();

  std::atomic<State> state_{State::Uncalibrated};
  std::atomic<bool> suspect_{false};
  std::atomic<uint32_t> position_{0};
  int32_t pending_ = 0;
  uint32_t max_position_ = 1;
  uint32_t max_steps_ = 0;
};

#endif
