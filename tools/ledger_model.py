#!/usr/bin/env python3
"""Exhaustive power-loss model of the hand-position record.

Two designs are modelled against the same harness:

  Original   position saved to NVS only by "Set hands" (calibrate) and once
             before each fast catch-up chunk; a normal one-minute tick never
             saves (HollowClock.cpp before the PositionLedger change).
  Ledger     every motion is bracketed by begin() and commit(), each of which
             writes the NVS word; a word still flagged MOVING at boot marks the
             record suspect (PositionLedger.cpp).

The harness runs a schedule of motions and cuts power after any subset of the
atomic events (an NVS write, which the ESP-IDF NVS library makes atomic, or a
single motor step). After each cut the design reloads its record and the
harness compares the belief with where the hands physically are.

Checked for every path:
  Ledger  P1  not suspect  =>  belief == physical           (silent error is 0)
          P2  suspect      =>  |physical - belief| <= K * interrupted_motions
          P3  suspect      <=> at least one motion was cut since calibrate
          P4  reloading never yields the Moving state (no stuck state)
  Both    the largest error, flagged and silent, over all paths

Run:  python3 tools/ledger_model.py
No dependencies. The counts it prints are the ones the explanation page quotes.
"""

from itertools import product

INVALID = 0xFFFFFFFF
FLAG_MOVING = 0x80000000
FLAG_SUSPECT = 0x40000000
MASK = 0x00FFFFFF


class Nvs:
    """One 32-bit word. A write either lands completely or not at all."""

    def __init__(self):
        self.word = INVALID


class OriginalDesign:
    name = "original"

    def __init__(self, nvs, max_position, max_steps):
        self.nvs, self.M = nvs, max_position
        self.load()

    def load(self):
        self.belief = None if self.nvs.word == INVALID else self.nvs.word
        self.suspect = False
        self.in_motion = 0

    def calibrate(self, position):
        self.belief = position % self.M
        self.nvs.word = self.belief

    # A normal tick: no write at all. (The fast catch-up wrote the pre-move
    # position; that is the same as the Ledger's begin() without the flag, and
    # it is not what a clock on a shelf does minute after minute.)
    def begin(self, steps):
        self.in_motion = steps
        return []  # no NVS event

    def commit(self):
        self.belief = (self.belief + self.in_motion) % self.M
        self.in_motion = 0
        return []  # no NVS event


class LedgerDesign:
    name = "ledger"

    def __init__(self, nvs, max_position, max_steps):
        self.nvs, self.M, self.K = nvs, max_position, max_steps
        self.load()

    def load(self):
        raw = self.nvs.word
        self.pending = 0
        if raw == INVALID or (raw & MASK) >= self.M:
            self.state, self.belief, self.suspect = "Uncalibrated", None, False
            return
        self.belief = raw & MASK
        self.suspect = bool(raw & (FLAG_SUSPECT | FLAG_MOVING))
        self.state = "Settled"

    def _word(self):
        raw = self.belief & MASK
        if self.state == "Moving":
            raw |= FLAG_MOVING
        if self.suspect:
            raw |= FLAG_SUSPECT
        return raw

    def calibrate(self, position):
        self.pending, self.suspect, self.state = 0, False, "Settled"
        self.belief = position % self.M
        return [("write", self._word())]

    def begin(self, steps):
        assert self.state == "Settled" and 0 < abs(steps) <= self.K
        self.pending, self.state = steps, "Moving"
        return [("write", self._word())]

    def commit(self):
        assert self.state == "Moving"
        self.belief = (self.belief + self.pending) % self.M
        self.pending, self.state = 0, "Settled"
        return [("write", self._word())]


def events_for(design, schedule, start):
    """Flatten a schedule into atomic events with the design's own writes.

    An event is ("write", word) or ("step", +1/-1) or ("calibrate", pos).
    The design object is driven along the way so its writes carry the words
    it would really write; physical motion is applied by the harness.
    """
    ev = []
    for item in schedule:
        if item[0] == "calibrate":
            ev.append(("calibrate", item[1]))
            continue
        steps = item[1]
        ev.extend(design.begin(steps))
        ev.extend([("step", 1 if steps > 0 else -1)] * abs(steps))
        ev.extend(design.commit())
    return ev


def run_path(design_cls, M, K, schedule, cuts, start):
    """Run one path. `cuts` is the set of event indices after which power dies.

    Returns a list of (belief, physical, suspect, interrupted, state) samples,
    one per reboot, plus the final sample.
    """
    nvs = Nvs()
    nvs.word = start
    design = design_cls(nvs, M, K)
    physical = start % M
    interrupted = 0
    samples = []
    i = 0
    pending_dir = 0
    in_motion = False
    # Re-derive the event list after every reboot, because the design's writes
    # depend on its own state (suspect flag, pending steps).
    remaining = list(schedule)
    while remaining:
        item = remaining[0]
        if item[0] == "calibrate":
            # The user sets the hands correctly: belief becomes the truth.
            for w in design.calibrate(physical):
                nvs.word = w[1]
            interrupted = 0
            if i in cuts:
                design.load()
                samples.append((design.belief, physical, design.suspect,
                                interrupted, getattr(design, "state", None)))
            i += 1
            remaining.pop(0)
            continue
        steps = item[1]
        motion_events = []
        motion_events.extend(design.begin(steps))
        motion_events.extend([("step", 1 if steps > 0 else -1)] * abs(steps))
        motion_events.extend(design.commit())
        cut_here = False
        moved = 0
        for ev in motion_events:
            if ev[0] == "write":
                nvs.word = ev[1]
            else:
                physical = (physical + ev[1]) % M
                moved += 1
            if i in cuts:
                cut_here = True
                i += 1
                break
            i += 1
        if cut_here:
            # The motion was cut unless every step had already landed and the
            # commit write had landed too (then nothing was lost).
            if nvs.word != motion_events[-1][1] if motion_events and motion_events[-1][0] == "write" else True:
                interrupted += 1
            design.load()
            samples.append((design.belief, physical, design.suspect,
                            interrupted, getattr(design, "state", None)))
            # The rest of this motion never happens; the clock will catch up
            # from its belief, which is exactly what the sample measures.
            remaining.pop(0)
            continue
        remaining.pop(0)
    samples.append((design.belief, physical, design.suspect, interrupted,
                    getattr(design, "state", None)))
    return samples


def signed_error(belief, physical, M):
    """physical - belief on the circle, in (-M/2, M/2]."""
    d = (physical - belief) % M
    return d - M if d > M // 2 else d


def check(design_cls, M, K, depth, backward, max_cuts):
    sizes = [s for s in range(1, K + 1)]
    if backward:
        sizes += [-s for s in sizes]
    paths = 0
    worst_silent = 0
    worst_flagged = 0
    violations = []
    for shape in product(sizes, repeat=depth):
        schedule = [("move", s) for s in shape]
        # every event boundary in a fully-run schedule; cuts are chosen among them
        n_events = sum(2 + abs(s) for s in shape) if design_cls is LedgerDesign \
            else sum(abs(s) for s in shape)
        for cut_count in range(0, max_cuts + 1):
            from itertools import combinations
            for cuts in combinations(range(n_events), cut_count):
                paths += 1
                samples = run_path(design_cls, M, K, schedule, set(cuts), 0)
                for belief, physical, suspect, interrupted, state in samples:
                    err = abs(signed_error(belief, physical, M))
                    if state == "Moving":
                        violations.append(("P4 stuck Moving", shape, cuts))
                    if suspect:
                        worst_flagged = max(worst_flagged, err)
                        if err > K * interrupted:
                            violations.append(("P2 bound", shape, cuts, err))
                        if interrupted == 0:
                            violations.append(("P3 false alarm", shape, cuts))
                    else:
                        worst_silent = max(worst_silent, err)
                        if design_cls is LedgerDesign and err != 0:
                            violations.append(("P1 silent error", shape, cuts, err))
                        if design_cls is LedgerDesign and interrupted:
                            violations.append(("P3 missed", shape, cuts))
    return paths, worst_silent, worst_flagged, violations


def main():
    M, K = 12, 3  # 12-step dial so wrap-around is exercised; motions of 1..3
    print(f"model: dial of {M} steps, motions of 1..{K} steps\n")
    print(f"{'design':10s} {'depth':>5s} {'backward':>8s} {'cuts':>4s} "
          f"{'paths':>8s} {'silent err':>10s} {'flagged err':>11s} violations")
    for design_cls in (OriginalDesign, LedgerDesign):
        for depth, backward, max_cuts in ((3, False, 99), (4, True, 2), (6, False, 1)):
            paths, silent, flagged, viol = check(design_cls, M, K, depth, backward, max_cuts)
            print(f"{design_cls.name:10s} {depth:5d} {str(backward):>8s} "
                  f"{('any' if max_cuts > 9 else str(max_cuts)):>4s} {paths:8d} "
                  f"{silent:10d} {flagged:11d} {len(viol)}")
            for v in viol[:5]:
                print("   ", v)
    # The calibrate path: suspect must clear and the belief must be exact.
    nvs = Nvs()
    d = LedgerDesign(nvs, M, K)
    nvs.word = (5 & MASK) | FLAG_SUSPECT
    d.load()
    assert d.suspect and d.belief == 5
    for w in d.calibrate(7):
        nvs.word = w[1]
    d.load()
    assert not d.suspect and d.belief == 7 and d.state == "Settled"
    # A word written by the original firmware (no flags) loads as trusted.
    nvs.word = 9
    d.load()
    assert d.state == "Settled" and not d.suspect and d.belief == 9
    print("\ncalibrate clears suspect; an original-firmware word loads trusted: ok")


if __name__ == "__main__":
    main()
