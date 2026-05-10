# elevator
A simple C++11 elevator(s) text-based simulation 

In the 80s an italian magazine called "Commodore Computer Club" launched a competition to create a program that could simulate an elevator on the Commodore 64. I was a child at the time and my computer skills were zero, but I always had the idea of trying it out as a game, so I decided to try again today with C ++ and STL. 

The simulated model requires the user to call the elevator by simply selecting the floor he wants to go to. The "management" receives the call and assigns it to the most appropriate elevator. This model is the one that allows more optimizations than traditional internal choice elevator and is exactly what is installed in the building where I work.

"Elevator" is intended as an educational project, it was a small gym to practice with threading, synchronization, collections and modeling.

This is a Windows/Linux multiplatform project.

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
| `callsGenerator.minDelayBetweenCallsMs` | The shortest wait between two generated calls. | Time is expressed in milliseconds: `1000` means 1 second. This value must not be higher than `maxDelayBetweenCallsMs`. |
| `callsGenerator.maxDelayBetweenCallsMs` | The longest wait between two generated calls. | Time is expressed in milliseconds. Increase it for a calmer simulation, decrease it for busier traffic. |
| `elevator.timeToReachTheNextFloorMs` | How long an elevator takes to move from one floor to the next. | Time is expressed in milliseconds. Use a value greater than `0`; higher values make elevators slower. |
| `elevator.enterAndExitTimeMs` | How long doors stay open while people enter and exit. | Time is expressed in milliseconds. Higher values make every stop last longer. |
| `log.traceLevel` | How much detail is printed in the log. | Use `Debug` for the most detail, then `Verbose`, `Info`, `Warning`, or `Error` for progressively quieter output. |
| `log.defaultLogType` | Where log messages are written. | Use `Screen` to show messages in the console. |

Possible future improvements:
- Unit and integration tests
- Statistics
- GUI
- Log to file
