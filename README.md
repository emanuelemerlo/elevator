# elevator
A simple C++ text-based simulation of one or more elevators.

In the 80s an italian magazine called "Commodore Computer Club" launched a competition to create a program that could simulate an elevator on the Commodore 64. I was a child at the time and my computer skills were zero, but I always had the idea of trying it out as a game, so I decided to try again today with C ++ and STL. 

The simulated model requires the user to call the elevator by simply selecting the floor he wants to go to. The "management" receives the call and assigns it to the most appropriate elevator. This model is the one that allows more optimizations than traditional internal choice elevator and is exactly what is installed in the building where I work.

"Elevator" is intended as an educational project, it was a small gym to practice with threading, synchronization, collections and modeling.

This is a Windows/Linux multiplatform project.

## Running the simulation

Build the program from the repository root. With the provided `makefile`, run:

```sh
make
```

On Windows, the VS Code build task produces `Elevator.exe`. The task compiles all
source files in `src`, including the statistics module.

Start the program and press `Enter` when you want to stop the simulation. At the end,
the screen is replaced by a final report with global statistics, anomaly indicators
and one row for each configured elevator.

## Configuration

Simulation parameters are loaded from `config.json` at startup, so values such as the
number of elevators, floors, call generation, timing and log verbosity can be changed
without recompiling.

Configuration fields:

| Setting | What it changes | How to choose a value |
| --- | --- | --- |
| `building.numberOfElevators` | How many elevators are available in the building. | Use `1` or more. Higher values make the system handle calls more easily. |
| `building.numberOfFloors` | How many floors the building has. | Use at least `2`. The first floor is `0`, so `5` means floors from `0` to `4`. |
| `callsGenerator.type` | How passenger calls are created. | Use `Random` for automatic random calls, or `Fixed` to replay a predefined set of calls. |
| `callsGenerator.numberOfCalls` | How many random calls are generated before the generator stops. | Used only with `Random`. Use values like `5`, `20` or `100`; use `4294967295` only when you want calls to continue until the program is stopped. |
| `callsGenerator.maxConcurrentCalls` | Maximum number of active calls/passengers in the simulated building. | Use a finite population-like limit such as `600`. When the limit is reached, the generator pauses until passengers complete their trips. |
| `callsGenerator.numberOfSimulationDays` | How many simulated days the random generator should run. | Use `1` or more. The generator stops when either this limit or `numberOfCalls` is reached. |
| `callsGenerator.simulationDayDurationMs` | Duration of one simulated 24-hour traffic cycle. | The random generator repeats a daily profile with peaks in the morning, around lunch and around dinner. `120000` means a full day lasts 2 real minutes. |
| `callsGenerator.minDelayBetweenCallsMs` | The shortest wait between two generated calls. | Time is expressed in milliseconds: `1000` means 1 second. This value must not be higher than `maxDelayBetweenCallsMs`. |
| `callsGenerator.maxDelayBetweenCallsMs` | The longest wait between two generated calls. | Time is expressed in milliseconds. Increase it for a calmer simulation, decrease it for busier traffic. |
| `elevator.timeToReachTheNextFloorMs` | How long an elevator takes to move from one floor to the next. | Time is expressed in milliseconds. Use a value greater than `0`; higher values make elevators slower. |
| `elevator.enterAndExitTimeMs` | How long doors stay open while people enter and exit. | Time is expressed in milliseconds. Higher values make every stop last longer. |
| `elevator.maxPeople` | Maximum number of people allowed in a cabin. | The dispatcher considers both people already on board and calls already assigned to the cabin, so elevators are not overbooked beyond full load. Cabins above 80% load are penalized. |
| `log.traceLevel` | How much detail is printed in the log. | Use `Debug` for the most detail, then `Verbose`, `Info`, `Warning`, or `Error` for progressively quieter output. |
| `log.defaultLogType` | Where log messages are written. | Use `Screen` for console only, `File` for file only, or `ScreenAndFile`/`Both` for both outputs. |
| `log.filePath` | File used when file logging is enabled. | Relative paths are resolved from the program working directory. The default is `elevator.log`. |

## Logging

The simulator can write logs to different destinations through `log.defaultLogType`:

| Value | Behavior |
| --- | --- |
| `Screen` | Default mode. Shows the dashboard and rolling log in the console. |
| `File` | Writes log messages only to `log.filePath`. Useful for unattended or repeatable runs. |
| `ScreenAndFile` | Shows the console UI and also writes the same log stream to file. |
| `Both` | Alias for `ScreenAndFile`. |

Example:

```json
"log": {
  "traceLevel": "Info",
  "defaultLogType": "ScreenAndFile",
  "filePath": "elevator.log"
}
```

When file logging is enabled, the log file is recreated at the first log write of
the run and then appended during the same execution. The final statistics report is
also written to the file after pending asynchronous log messages are flushed.

## Elevator assignment

When a passenger call is generated, the management system scores every elevator and
assigns the call to the best candidate. The score is designed to reduce passenger
waiting time while avoiding inefficient or unfair use of the cabins.

The assignment logic considers:

| Factor | Meaning |
| --- | --- |
| Distance | Number of floors between the elevator and the caller. |
| Direction | Whether the elevator is already moving in the same direction requested by the caller. |
| Relative position | Whether the elevator has already passed the caller's floor. |
| Queued stops | Intermediate stops already scheduled before reaching the caller. |
| Motion status | Idle elevators are favored; elevators already serving passengers are evaluated by route cost. |
| Cabin load | Full elevators are excluded. Elevators above 80% capacity are penalized. |
| Wear leveling | Travelled floors, runs and door cycles add a small penalty so work is distributed over time. |

If no elevator is considered directly available, the system can still make a forced
assignment to the best fallback candidate. Forced assignments are reported as anomaly
indicators in the final statistics.

## Final statistics

When the simulation stops, the dashboard is replaced by a full final report. This
report is not limited by the normal rolling log window, so all configured elevators
are shown.
Long table values are wrapped onto continuation rows so recommendation reasons and
other explanatory fields keep the report aligned instead of breaking the table
layout.

If file logging is enabled, the same final report is appended to the configured log
file. This is useful when the screen report is long or when you want to compare
multiple runs after changing `config.json`.

The first table summarizes passenger flow:

| Metric | Meaning |
| --- | --- |
| `Queued calls` | Calls created and inserted into the waiting queue. |
| `Assigned calls` | Calls accepted by an elevator. |
| `Boarded passengers` | Passengers who entered an elevator. |
| `Completed passengers` | Passengers who reached their destination floor. |
| `Simulation time` | Current simulated day and time of day when the report is printed. The same value is shown live in the symbolic interface. |
| `Average wait` | Average time from call creation to passenger boarding, shown in seconds. |
| `Max wait` | Longest observed waiting time, shown in seconds. |

The anomaly table highlights situations worth investigating:

| Indicator | Meaning |
| --- | --- |
| `Forced assignments` | Calls assigned even though the normal availability rules did not find an ideal cabin. |
| `No available elevator` | Dispatch decisions where all cabins were busy, full, out of direction or otherwise not directly available. |
| `Capacity limit waits` | Dispatch attempts that had to wait because every cabin had already reached its configured committed load. |
| `Pending boarding` | Calls still waiting when the simulation was stopped. |
| `Pending completion` | Passengers still inside elevators when the simulation was stopped. |
| `Avg assignment time` | Average time spent by the dispatch algorithm to assign a call. |
| `Max assignment time` | Slowest dispatch decision observed. |
| `Max wait / avg wait` | Tail-latency indicator: high values mean some users waited much longer than average. |

Each indicator has a simple level:

| Level | Meaning |
| --- | --- |
| `OK` | Normal condition. |
| `INFO` | Not necessarily a problem, but useful context. |
| `WARN` | Potential anomaly or inefficiency to inspect. |

The final elevator table shows one row per configured elevator:

| Metric | Meaning |
| --- | --- |
| `Assigned calls` | Number of calls assigned to that elevator. Useful to detect load imbalance. |
| `Travelled floors` | Total floors travelled by the cabin. |
| `Runs` | Number of movement runs started. |
| `Door cycles` | Number of door close cycles, a proxy for door wear. |
| `Queued stops` | Stops still queued when the report is printed. |
| `People on board` | Passengers still inside the cabin when the report is printed. |

The report also includes a fleet sizing recommendation:

| Parameter | Meaning |
| --- | --- |
| `Configured elevators` | Number of elevators from `building.numberOfElevators`. |
| `Recommended elevators` | Suggested number of elevators for the observed run. |
| `Sizing reason` | Short explanation of the recommendation. |
| `Target avg wait` | Heuristic service target derived from movement and stop timing. |
| `Observed avg wait` | Average wait measured during the run. |
| `Forced assignment rate` | Share of dispatch decisions that required a fallback. |
| `No-available rate` | Share of dispatch decisions where no elevator was directly available. |
| `Unassigned elevators` | Elevators that received no calls during the run. |

The recommendation is conservative: it suggests adding elevators when waits,
backlog or fallback rates indicate pressure; it suggests reducing only when service
is comfortable and at least one elevator did no useful work.

The report also includes a cabin sizing recommendation:

| Parameter | Meaning |
| --- | --- |
| `Configured max people` | Current cabin capacity from `elevator.maxPeople`. |
| `Recommended max people` | Suggested number of people each cabin should carry for the observed load. |
| `Sizing reason` | Short explanation of the recommendation. |
| `Peak people on board` | Highest observed number of passengers physically inside a cabin. |
| `Peak committed load` | Highest observed cabin load including both people on board and people assigned but not boarded yet. |
| `Capacity limit waits` | Number of times dispatch had to wait because all cabins were at the configured committed load. |

Possible future improvements:
- Unit and integration tests
- GUI
- Log to file
