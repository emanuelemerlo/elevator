/**********************************************************************************
*        File: Configuration.h
* Description: Simulation parameters loaded from JSON file.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
**********************************************************************************/

#pragma once

#include <chrono>
#include <string>

#include "ILog.h"

namespace Configuration
{
  struct Building
  {
    unsigned int numberOfElevators = 1;
    unsigned int numberOfFloors = 5;
  };

  struct CallsGenerator
  {
    enum class Type { Fixed, Random };

    static constexpr unsigned int EndlessCalls = static_cast<unsigned int>(-1);

    Type generatorType = Type::Random;
    unsigned int numberOfCalls = 5;
    long long minDelayBetweenCallsMs = 3000;
    long long maxDelayBetweenCallsMs = 10000;
  };

  struct Elevator
  {
    std::chrono::milliseconds timeToReachNextFloor{ 2000 };
    std::chrono::milliseconds enterAndExitTime{ 2000 };
  };

  struct Log
  {
    ILog::TraceLevel traceLevel = ILog::TraceLevel::Verbose;
    ILog::LogType defaultLogType = ILog::LogType::Screen;
  };

  struct Settings
  {
    Building building;
    CallsGenerator callsGenerator;
    Elevator elevator;
    Log log;
  };

  const Settings& Get();
  bool LoadFromJsonFile(const std::string& path = "config.json");
}
