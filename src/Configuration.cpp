#include "Configuration.h"

#include <fstream>
#include <regex>
#include <sstream>

namespace Configuration
{
  namespace
  {
    Settings g_settings{};

    std::string ReadFile(const std::string& path)
    {
      std::ifstream file(path);
      if (!file)
        return "";

      std::stringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    }

    bool ExtractUnsigned(const std::string& source, const std::string& key, unsigned int& output)
    {
      const std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
      std::smatch match;
      if (!std::regex_search(source, match, pattern) || match.size() < 2)
        return false;

      output = static_cast<unsigned int>(std::stoul(match[1].str()));
      return true;
    }

    bool ExtractLongLong(const std::string& source, const std::string& key, long long& output)
    {
      const std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
      std::smatch match;
      if (!std::regex_search(source, match, pattern) || match.size() < 2)
        return false;

      output = std::stoll(match[1].str());
      return true;
    }

    bool ExtractString(const std::string& source, const std::string& key, std::string& output)
    {
      const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
      std::smatch match;
      if (!std::regex_search(source, match, pattern) || match.size() < 2)
        return false;

      output = match[1].str();
      return true;
    }

    CallsGenerator::Type ParseGeneratorType(const std::string& value)
    {
      return value == "Fixed" ? CallsGenerator::Type::Fixed : CallsGenerator::Type::Random;
    }

    ILog::TraceLevel ParseTraceLevel(const std::string& value)
    {
      if (value == "Debug") return ILog::TraceLevel::Debug;
      if (value == "Info") return ILog::TraceLevel::Info;
      if (value == "Warning") return ILog::TraceLevel::Warning;
      if (value == "Error") return ILog::TraceLevel::Error;
      return ILog::TraceLevel::Verbose;
    }

    ILog::LogType ParseLogType(const std::string& value)
    {
      return value == "File" ? ILog::LogType::File : ILog::LogType::Screen;
    }
  }

  const Settings& Get()
  {
    return g_settings;
  }

  bool LoadFromJsonFile(const std::string& path)
  {
    const auto json = ReadFile(path);
    if (json.empty())
      return false;

    ExtractUnsigned(json, "numberOfElevators", g_settings.building.numberOfElevators);
    ExtractUnsigned(json, "numberOfFloors", g_settings.building.numberOfFloors);

    unsigned int numberOfCalls = 0;
    if (ExtractUnsigned(json, "numberOfCalls", numberOfCalls))
      g_settings.callsGenerator.numberOfCalls = numberOfCalls;

    ExtractLongLong(json, "minDelayBetweenCallsMs", g_settings.callsGenerator.minDelayBetweenCallsMs);
    ExtractLongLong(json, "maxDelayBetweenCallsMs", g_settings.callsGenerator.maxDelayBetweenCallsMs);

    long long timeToReachNextFloorMs = g_settings.elevator.timeToReachNextFloor.count();
    if (ExtractLongLong(json, "timeToReachNextFloorMs", timeToReachNextFloorMs))
      g_settings.elevator.timeToReachNextFloor = std::chrono::milliseconds(timeToReachNextFloorMs);

    long long enterAndExitTimeMs = g_settings.elevator.enterAndExitTime.count();
    if (ExtractLongLong(json, "enterAndExitTimeMs", enterAndExitTimeMs))
      g_settings.elevator.enterAndExitTime = std::chrono::milliseconds(enterAndExitTimeMs);

    std::string generatorType;
    if (ExtractString(json, "generatorType", generatorType))
      g_settings.callsGenerator.generatorType = ParseGeneratorType(generatorType);

    std::string traceLevel;
    if (ExtractString(json, "traceLevel", traceLevel))
      g_settings.log.traceLevel = ParseTraceLevel(traceLevel);

    std::string defaultLogType;
    if (ExtractString(json, "defaultLogType", defaultLogType))
      g_settings.log.defaultLogType = ParseLogType(defaultLogType);

    return true;
  }
}
