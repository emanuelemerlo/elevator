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
    std::chrono::milliseconds minDelayBetweenCalls{ std::chrono::seconds(3) };
    std::chrono::milliseconds maxDelayBetweenCalls{ std::chrono::seconds(10) };

    std::chrono::milliseconds timeToReachTheNextFloor{ std::chrono::seconds(2) };
    std::chrono::milliseconds enterAndExitTime{ std::chrono::seconds(2) };

    ILog::TraceLevel traceLevel{ ILog::TraceLevel::Verbose };
    ILog::LogType defaultLogType{ ILog::LogType::Screen };
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
  }

  namespace CallsGenerator
  {
    Type GeneratorType();
    unsigned int NumberOfCalls();
    long long MinDelayBetweenCalls();
    long long MaxDelayBetweenCalls();
  }

  namespace Log
  {
    ILog::TraceLevel TraceLevel();
    ILog::LogType DefaultLogType();
  }
}
