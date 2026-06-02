/**********************************************************************************
*        File: Configuration.h
* Description: Runtime simulation parameters.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include "ILog.h"

#include <chrono>
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

    /**
     * \brief Assigned to NumberOfCalls specify that the generator must generate continuous calls.
     */
    constexpr unsigned int EndlessCalls = static_cast<unsigned int>(-1);
  }

  struct Settings
  {
    unsigned int numberOfElevators{ 3 };
    unsigned int numberOfFloors{ 5 };

    CallsGenerator::Type generatorType{ CallsGenerator::Type::Random };
    unsigned int numberOfCalls{ 5 };
    unsigned int maxConcurrentCalls{ 600 };
    unsigned int numberOfSimulationDays{ 1 };
    std::chrono::milliseconds simulationDayDuration{ std::chrono::minutes(2) };
    std::chrono::milliseconds minDelayBetweenCalls{ std::chrono::seconds(3) };
    std::chrono::milliseconds maxDelayBetweenCalls{ std::chrono::seconds(10) };

    std::chrono::milliseconds timeToReachTheNextFloor{ std::chrono::seconds(2) };
    std::chrono::milliseconds enterAndExitTime{ std::chrono::seconds(2) };
    unsigned int maxPeoplePerElevator{ 8 };

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
    unsigned int MaxPeople();
  }

  namespace CallsGenerator
  {
    Type GeneratorType();
    unsigned int NumberOfCalls();
    unsigned int MaxConcurrentCalls();
    unsigned int NumberOfSimulationDays();
    long long SimulationDayDuration();
    long long MinDelayBetweenCalls();
    long long MaxDelayBetweenCalls();
  }

  namespace Simulation
  {
    void StartClock();
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
