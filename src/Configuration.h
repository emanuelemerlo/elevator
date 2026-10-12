/**********************************************************************************
*        File: Configuration.h
* Description: Runtime simulation parameters.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include "ILog.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <limits>
#include <string>

/**
 * \brief Simulation parameters.
 */
namespace Configuration
{
  namespace CallsGenerator
  {
    /**
     * \brief Types of calls generators.
     */
    enum class Type { Fixed, Random };

  }

  struct Settings
  {
    unsigned int numberOfElevators{ 3 };
    unsigned int numberOfFloors{ 5 };

    CallsGenerator::Type generatorType{ CallsGenerator::Type::Random };
    unsigned int numberOfCalls{ std::numeric_limits<unsigned int>::max() }; // Random generator: stop after this many calls
    unsigned int averageCallsPerDay{ 600 };
    unsigned int maxConcurrentCalls{ 600 };
    unsigned int numberOfSimulationDays{ 1 };
    std::chrono::milliseconds simulationDayDuration{ std::chrono::minutes(2) };
    std::chrono::milliseconds minDelayBetweenCalls{ std::chrono::seconds(3) };
    std::chrono::milliseconds maxDelayBetweenCalls{ std::chrono::seconds(10) };

    std::chrono::milliseconds timeToReachTheNextFloor{ std::chrono::seconds(2) };
    std::chrono::milliseconds enterAndExitTime{ std::chrono::seconds(2) };
    unsigned int maxPeoplePerElevator{ 8 };
    // Time needed by the doors to open or to close (each transition), in simulated milliseconds.
    std::chrono::milliseconds doorTransitionTime{ std::chrono::seconds(1) };

    // Simulation speed: 1.0 = real time, 10.0 = the simulated clock runs ten times faster than the wall clock.
    // Every configured duration stays expressed in simulated time, so the results are comparable across speeds.
    double timeScale{ 1.0 };
    // Seed of the random calls generator: 0 = pick a different seed at every run.
    unsigned int randomSeed{ 0 };
    // Stop the simulation (and print the final report) as soon as all calls have been generated and served.
    bool autoStop{ true };

    ILog::TraceLevel traceLevel{ ILog::TraceLevel::Verbose };
    ILog::LogType defaultLogType{ ILog::LogType::Screen };
    std::string logFilePath{ "elevator.log" };
  };

  bool LoadFromFile(const std::string& path);
  const Settings& Get();

  namespace Building
  {
    unsigned int NumberOfElevators();
    unsigned int NumberOfFloors();
  }

  namespace Elevator
  {
    std::chrono::milliseconds TimeToReachTheNextFloor();
    std::chrono::milliseconds EnterAndExitTime();
    std::chrono::milliseconds DoorTransitionTime();
    unsigned int MaxPeople();
  }

  namespace CallsGenerator
  {
    Type GeneratorType();
    unsigned int NumberOfCalls();
    unsigned int AverageCallsPerDay();
    unsigned int MaxConcurrentCalls();
    unsigned int NumberOfSimulationDays();
    long long SimulationDayDuration();
    long long MinDelayBetweenCalls();
    long long MaxDelayBetweenCalls();
  }

  namespace Simulation
  {
    void StartClock();

    double TimeScale();
    unsigned int RandomSeed();
    bool AutoStop();

    /** \brief Converts a simulated duration into the real time to wait for it (honours the time scale). */
    std::chrono::microseconds ToRealDuration(std::chrono::milliseconds simulatedDuration);
    /** \brief Converts a measured real duration into simulated milliseconds (honours the time scale). */
    std::uint64_t ToSimulatedMs(std::chrono::microseconds realDuration);
    /** \brief Sleeps for a simulated duration. */
    void SleepFor(std::chrono::milliseconds simulatedDuration);
    /** \brief Simulated time elapsed since StartClock(), in milliseconds. */
    long long ElapsedMs();

    double CurrentHour();
    unsigned int CurrentDay();
    bool Completed();
    std::string CurrentDayTimeLabel();
  }

  namespace Log
  {
    ILog::TraceLevel TraceLevel();
    ILog::LogType DefaultLogType();
    std::string FilePath();
    bool WriteToScreen();
    bool WriteToFile();
  }
}
