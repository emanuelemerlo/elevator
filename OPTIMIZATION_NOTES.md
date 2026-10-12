# Dispatch and simulation improvements

This document replaces the previous notes, whose gains were estimates that had never been measured.
Everything below was measured with the simulator itself (see "How it was measured").

## Problems found in the previous version

| Problem | Effect |
| --- | --- |
| Doors opened and closed with a hard coded `sleep_for(1s)`, ignoring `config.json`, while the dispatcher assumed `enterAndExitTimeMs` (20 ms) per stop | Predictions were off by ~100x: the cost of a stop was invisible to the dispatcher |
| `EstimateRealisticPassengerLoad` returned 1 for an empty cabin, whatever was assigned to it | Overbooking: "Peak committed load" 23 with `maxPeople` 20 |
| Wear score grew without limit (door cycles x 12, runs x 8, ...) and was added to a time-like score | After a few hundred stops the wear dominated the distance |
| A passenger refused by a full cabin kept an assignment but his pickup stop had already been cleared | Passenger stranded: 46-51 calls never served in the small-cabin scenario |
| The destination stop was flagged when the call was assigned | Empty stops at destination floors before the passenger boarded |
| Fixed 200 ms polling of the pending queue, first-in-first-out blocking | A call that did not fit held back the ones that did |
| Random seed, real time only, Enter required to stop, only average/max wait | Runs could not be repeated, compared or automated |

## What changed

- **Dispatcher** (`AssignmentOptimizer`): the route of every cabin is simulated with and without the new call, using the
  real stop rule (`Floors::FindNextStop`) and configured times. Cost = extra weighted passenger time (own wait x2 + ride +
  delay inflicted to passengers already assigned, protected by an aging weight). Capacity is exact (never more than
  `maxPeople` people at once). Wear is only a tie-breaker, capped at one floor of travel.
- **Prediction fidelity**: the model knows that a cabin does not re-evaluate a destination it has already chosen
  (`ElevatorDispatchState::targetFloor`), that it stops at closer stops flagged for its direction while travelling, and
  that with doors open it must finish the stop in progress. Each of these was a real mismatch found through the logs
  (`Dispatch state of ...` at `Debug` level shows what the prediction was based on).
- **Cabins**: configurable door time, only the pickup floor is flagged at assignment, refused passengers go back to the
  dispatcher, capacity changes wake the pending queue.
- **Simulation**: `timeScale`, `randomSeed`, `autoStop`, wait percentiles, journey time, oldest waiting call,
  requeued calls. Sizing heuristics no longer treat a sweep reversal as an anomaly.
- **Tests**: `make test` (22 checks on the route simulation and floor logic).

## Measured results

Same machine (one CPU), real time (`timeScale` 1), call generation limited by the same timing rules.
The original code was run 2-3 times per scenario with a random seed, the new one twice with seeds 1 and 2.
Times are seconds of simulated time. "Never served" = calls still on a floor when the run was stopped.

| Scenario | Version | Average wait | Max wait | p95 wait | Never served | Refused boardings |
| --- | --- | --- | --- | --- | --- | --- |
| S1: 10 elevators, 10 floors, 20 people (project config, 2 days x 30 s, 290 calls) | original | 14.6 - 18.0 | 68.5 - 75.0 | n/a | 0 | n/a |
| | new | 3.1 - 3.4 | 12.2 - 15.5 | 6.8 - 7.7 | 0 | 0 |
| S2: 4 elevators, 15 floors, 8 people (73 calls) | original | 18.0 - 20.1 | 54.6 - 64.4 | n/a | 0 - 1 | n/a |
| | new | 7.6 - 9.2 | 24.7 - 32.1 | 21.1 - 23.7 | 0 | 0 |
| S3: 3 elevators, 8 floors, 4 people, heavy traffic (110 calls) | original | 14.0 - 17.5 (*) | 44.9 - 45.0 (*) | n/a | 46 - 51 | n/a |
| | new | 20.4 - 23.8 | 54.4 - 59.9 | 46.4 - 57.3 | 0 | 0 |

(*) The original averages only count the passengers that boarded (59-62 of 110), so they look better than they are:
in S3 the new version serves every passenger, the original leaves about half of them waiting.
In S1/S2 the original reports no percentiles or journey time; the new version averages 7.8 - 8.1 s (S1) and
14.8 - 15.5 s (S2) from call to arrival.

Limits of these numbers: two to three runs per cell, one machine with a single CPU (thread timing is noisy), and
S2/S3 are scenarios I defined to stress the dispatcher, not real building traffic. `timeScale` 4 gave results consistent
with real time on S1 (4.61 s vs 4.71 s average wait, before the last fidelity fixes).

## How it was measured

Configurations: `log.defaultLogType = File`, `traceLevel = Warning`, same `config.json` for both versions; the original
was stopped with Enter after the traffic had ended and the cabins had time to drain.

## Not done

- Idle cabins parking (spreading empty cabins or sending them to the lobby before the morning peak).
- Door dwell is not extended when people are still entering; the doors are not closed early.
- The route simulation assumes the cabin starts from the last floor it reached (it ignores a partial floor in progress).
