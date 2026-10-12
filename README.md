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

Start the program: by default it stops by itself as soon as every call has been generated and
served (`simulation.autoStop`). You can still press `Enter` at any time to stop it earlier. At the end,
the screen is replaced by a final report with global statistics, anomaly indicators
and one row for each configured elevator.

To compare two dispatching strategies or two configurations quickly and repeatably, combine
`simulation.timeScale` (run faster than real time), `callsGenerator.randomSeed` (same calls every run) and
`log.defaultLogType: "File"` (no dashboard, the final report is written to the log file).

### Tests

```sh
make test
```

builds and runs `tests/AssignmentOptimizerTests.cpp`, which checks the route simulation used by the dispatcher
(arrival times, sweep direction, capacity, shared stops, anti-starvation weight) and the stop logic of the floors.

## Configuration

Simulation parameters are loaded from `config.json` at startup, so values such as the
number of elevators, floors, call generation, timing and log verbosity can be changed
without recompiling.

Configuration fields. In the table below the dotted names (for example
`building.numberOfElevators`) only indicate the logical group of each setting: the shipped
`config.json` is a flat object and uses just the last part (`numberOfElevators`). Keys are looked up
by name, so a nested layout with the same key names is accepted too. Invalid values (negative,
out of range, unknown enum names) stop the program at startup with an explanatory message.

| Setting | What it changes | How to choose a value |
| --- | --- | --- |
| `building.numberOfElevators` | How many elevators are available in the building. | Use `1` or more. Higher values make the system handle calls more easily. |
| `building.numberOfFloors` | How many floors the building has. | Use at least `2`. The first floor is `0`, so `5` means floors from `0` to `4`. |
| `callsGenerator.type` | How passenger calls are created. | Use `Random` for automatic random calls, or `Fixed` to replay a predefined set of calls. |
| `callsGenerator.numberOfCalls` | How many random calls are generated before the generator stops. | Used only with `Random`. Use values like `5`, `20` or `100`; use `4294967295` only when you want calls to continue until the program is stopped. |
| `callsGenerator.averageCallsPerDay` | Average number of calls generated in one simulated day: it sets the base rate of the daily traffic profile. | Used only with `Random`. Default `600`. The resulting delay between two calls is always kept between `minDelayBetweenCallsMs` and `maxDelayBetweenCallsMs`. |
| `callsGenerator.maxConcurrentCalls` | Maximum number of active calls/passengers in the simulated building. | Use a finite population-like limit such as `600`. When the limit is reached, the generator pauses until passengers complete their trips. |
| `callsGenerator.numberOfSimulationDays` | How many simulated days the random generator should run. | Use `1` or more. The generator stops when either this limit or `numberOfCalls` is reached. |
| `callsGenerator.simulationDayDurationMs` | Duration of one simulated 24-hour traffic cycle. | The random generator repeats a daily profile with peaks in the morning, around lunch and around dinner. `120000` means a full day lasts 2 real minutes. |
| `callsGenerator.minDelayBetweenCallsMs` | The shortest wait between two generated calls. | Time is expressed in milliseconds: `1000` means 1 second. This value must not be higher than `maxDelayBetweenCallsMs`. |
| `callsGenerator.maxDelayBetweenCallsMs` | The longest wait between two generated calls. | Time is expressed in milliseconds. Increase it for a calmer simulation, decrease it for busier traffic. |
| `elevator.timeToReachTheNextFloorMs` | How long an elevator takes to move from one floor to the next. | Time is expressed in milliseconds. Use a value greater than `0`; higher values make elevators slower. |
| `elevator.enterAndExitTimeMs` | How long doors stay open while people enter and exit. | Time is expressed in milliseconds. Higher values make every stop last longer. |
| `elevator.doorTransitionTimeMs` | How long the doors take to open, and to close (each transition). | Time is expressed in milliseconds. Default `1000`. Every stop costs one opening and one closing, so with fast floors this value dominates the trip time. It is used both by the cabins and by the dispatcher prediction. |
| `elevator.maxPeople` | Maximum number of people allowed in a cabin. | The dispatcher simulates the route of every cabin and never assigns a call that would make more than `maxPeople` people ride together. Passengers whose rides do not overlap do not count against each other. |
| `simulation.timeScale` | Speed of the simulation compared to real time. | Default `1` (real time). `10` runs ten times faster. All durations in this file stay expressed in simulated time and the report is in simulated time, so results are comparable between speeds; very high values (above ~20 with short timings) lose precision because of the operating system timers. |
| `callsGenerator.randomSeed` | Seed of the random calls generator. | `0` (default) picks a different seed at every run. Use a fixed value to get the same sequence of floors and delays. The seed used is written to the log. The threads timing is not deterministic, so results of two runs are close but not identical. |
| `simulation.autoStop` | Stops the program when the simulation is over. | `true` (default): when all calls have been generated and served the final report is printed and the program exits. `false`: the program runs until `Enter` is pressed. |
| `log.traceLevel` | How much detail is printed in the log. | Use `Debug` for the most detail, then `Verbose`, `Info`, `Warning`, or `Error` for progressively quieter output. |
| `log.defaultLogType` | Where log messages are written. | Use `Screen` for console only, `File` for file only, or `ScreenAndFile`/`Both` for both outputs. |
| `log.filePath` | File used when file logging is enabled. | Relative paths are resolved from the program working directory. The default is `elevator.log`. |

## Logging

The simulator can write logs to different destinations through `log.defaultLogType`:

| Value | Behavior |
| --- | --- |
| `Screen` | Default mode. Shows the dashboard and rolling log in the console. |
| `File` | Writes log messages only to `log.filePath`; the console dashboard is not drawn. Useful for unattended or repeatable runs. |
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

When a passenger call is generated, the management system decides which elevator takes it by
**simulating the route of every cabin**, with and without the new call, and choosing the cabin for which the
call costs the least total passenger time.

The simulation uses the same stop-selection rule as the real cabins (`Floors::FindNextStop`), the configured
floor, door and boarding times, and the exact set of passengers already inside each cabin or assigned to it and still
waiting. For every cabin it computes when each passenger will be picked up and dropped off.

The cost of a candidate is the extra weighted passenger time caused by the call:

| Factor | Meaning |
| --- | --- |
| Own wait and ride | Time until the new passenger is picked up (weighted x2: waiting on a floor hurts more) and then delivered. |
| Delay inflicted to others | Passengers already assigned to that cabin that get picked up or delivered later because of the new stops. Joining a stop that is already scheduled is nearly free. |
| Waiting age | The delay inflicted to a passenger who has already waited a long time counts up to 4 times more, so nobody is postponed over and over (starvation). |
| Capacity | Exact: the simulated route must never carry more than `maxPeople` people at the same time. Cabins that would exceed it are excluded; if all of them are, the call waits in the pending queue. |
| Wear leveling | Travelled floors, runs and door cycles add a tiny penalty, normalized among the candidates and never worth more than one floor of travel. It only breaks ties. |

If no cabin can take the call because of its capacity, the call goes to a pending queue that is retried
as soon as passengers leave a cabin (and periodically). A passenger who finds a full cabin anyway
(boarding refused) is automatically given back to the dispatcher instead of being left on the floor.

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
| `Wait p50` / `Wait p95` / `Wait p99` | Median, 95th and 99th percentile of the waiting time: how long *most* passengers wait and how bad the tail is. |
| `Average journey` / `Max journey` | Time from the call to the arrival at the destination (wait + ride). |

All times are simulated times (see `simulation.timeScale`).

The anomaly table highlights situations worth investigating:

| Indicator | Meaning |
| --- | --- |
| `Forced assignments` | Calls assigned to a cabin that had to reverse its sweep before picking the passenger up. Normal in a busy building, so it is context (`INFO`), not an anomaly. |
| `No available elevator` | Dispatch decisions where every cabin had to reverse its sweep: the saturation signal (`WARN` above 10% of the decisions). |
| `Requeued calls` | Passengers who found a full cabin and were dispatched again. It should be `0`: the dispatcher prevents it by simulating the load. |
| `Oldest waiting call` | Waiting time of the call still on a floor for the longest time when the report is printed. |
| `Capacity limit waits` | Calls that had to wait in the pending queue because every cabin had already reached its configured committed load (counted once per call, not once per retry). |
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
| `Forced assignment rate` | Share of dispatch decisions that required a reversal (information only, not used for the recommendation). |
| `No-available rate` | Share of dispatch decisions where every elevator had to reverse (used for the recommendation). |
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
- Idle cabins parking (e.g. spread the empty cabins, send them to the lobby before the morning peak)
- Integration tests of whole simulation runs
- GUI
