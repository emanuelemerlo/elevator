# elevator
A simple C++11 elevator(s) text-based simulation 

In the 80s an italian magazine called "Commodore Computer Club" launched a competition to create a program that could simulate an elevator on the Commodore 64. I was a child at the time and my computer skills were zero, but I always had the idea of trying it out as a game, so I decided to try again today with C ++ and STL. 

The simulated model requires the user to call the elevator by simply selecting the floor he wants to go to. The "management" receives the call and assigns it to the most appropriate elevator. This model is the one that allows more optimizations than traditional internal choice elevator and is exactly what is installed in the building where I work.

"Elevator" is intended as an educational project, it was a small gym to practice with threading, synchronization, collections and modeling.

This is a Windows/Linux multiplatform project.

Possible future improvements:
- Unit and integration tests
- Statistics
- GUI
- Log to file

## Configuration

Runtime configuration is now loaded from `config.json` at startup.

You can tune:
- building size (`numberOfElevators`, `numberOfFloors`),
- calls generator behavior (type, count, delays),
- elevator timings,
- log level/type.

If the file is missing, the simulator falls back to built-in defaults.

## Quick functional verification

A non-interactive smoke test is available for quick functional verification:

```bash
make smoke-test
```

The smoke test builds the project, runs the simulation by automatically sending `Enter` after 3 seconds, and verifies that the startup/shutdown lifecycle completes correctly.

## Proposed improvements

- Add deterministic integration tests (e.g., fixed seed and reproducible traffic scenarios).
- Move runtime parameters from `Configuration.h` to a configuration file (JSON/TOML).
- Add automatic metrics (average waiting time, throughput, percentiles).
- Save logs to file with rotation and configurable levels.
- Add CI (GitHub Actions) with Linux/Windows builds and automatic smoke test.

## Elevator selection algorithm and threading verification (updated)

During functional verification, two critical issues were found and fixed:

- **Elevator selection**: floor-distance sorting used the wrong direction (`>`), favoring the farthest elevator instead of the nearest one.
- **Concurrency in call assignment**: async assignment captured a local iterator by reference (`&it`), with a use-after-scope/data-race risk.

Applied fixes:

- Corrected ascending distance ordering in `Management::AssignCall`.
- Added mutex protection in `Management::AssignCall` and `Management::Shutdown`.
- Switched to safe call capture in async lambdas (`shared_ptr` by value).

## Activity log improvement

Yes: the runtime log format was improved to make concurrent analysis and debugging easier.
Each log line now includes:

- relative time from logging start (ms),
- level (`DBG/VRB/INF/WRN/ERR`),
- producer thread id,
- source (`traceId`) and event text.

Example:

```text
[+  123ms][INF][T:140232841696000] Elevator A | Doors open
```
