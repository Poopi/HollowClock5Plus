#include "PositionLedger.h"
#include "PreferencesManager.h"

void PositionLedger::load(uint32_t max_position, uint32_t max_steps_per_move) {
  max_position_ = max_position ? max_position : 1;
  max_steps_ = max_steps_per_move;
  pending_ = 0;

  uint32_t raw = PreferencesManager::getInstance().getClockPosition();
  uint32_t position = raw & POSITION_MASK;
  if (raw == PreferencesManager::INVALID_CLOCK_POSITION ||
      position >= max_position_) {
    state_ = State::Uncalibrated;
    suspect_ = false;
    position_ = 0;
    return;
  }
  // A record still flagged Moving means power was lost between begin() and
  // commit(). The hands are somewhere within one motion of the recorded
  // position; the record is kept as the belief and the doubt is remembered.
  position_ = position;
  suspect_ = (raw & (FLAG_SUSPECT | FLAG_MOVING)) != 0;
  state_ = State::Settled;
}

uint32_t PositionLedger::wrap(int64_t position) const {
  int64_t m = max_position_;
  return (uint32_t)(((position % m) + m) % m);
}

void PositionLedger::store() {
  uint32_t raw = position_.load() & POSITION_MASK;
  if (state_.load() == State::Moving) {
    raw |= FLAG_MOVING;
  }
  if (suspect_.load()) {
    raw |= FLAG_SUSPECT;
  }
  PreferencesManager::getInstance().setClock(raw);
}

void PositionLedger::calibrate(uint32_t position) {
  pending_ = 0;
  position_ = wrap(position);
  suspect_ = false;
  state_ = State::Settled;
  store();
}

bool PositionLedger::begin(int32_t steps) {
  if (state_.load() != State::Settled || steps == 0 ||
      (uint32_t)(steps < 0 ? -steps : steps) > max_steps_) {
    return false;
  }
  pending_ = steps;
  state_ = State::Moving;
  store();
  return true;
}

void PositionLedger::commit() {
  if (state_.load() != State::Moving) {
    return;
  }
  position_ = wrap((int64_t)position_.load() + pending_);
  pending_ = 0;
  state_ = State::Settled;
  store();
}
