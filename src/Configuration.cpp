#include "Configuration.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <limits>

namespace
{
  Configuration::Settings g_settings;
  std::chrono::steady_clock::time_point g_simulationStartTime = std::chrono::steady_clock::now();

  std::string ReadFile(const std::string& path)
  {
    std::ifstream file(path);
    if (!file.is_open())
      return {};

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  std::string ToLower(std::string value)
  {
    std::transform(value.begin(), value.end(), value.begin(),
      [](const unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
  }

  bool FindUnsigned(const std::string& json, const std::string& key, unsigned int& value)
  {
    // A leading '-' is matched on purpose: a negative value must be reported as an
    // error instead of being silently ignored (and replaced by the default).
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern))
      return false;

    const auto text = match[1].str();
    if (text.front() == '-')
      throw std::invalid_argument("Invalid value for '" + key + "': it must not be negative");

    const auto parsed = std::stoull(text);
    if (parsed > std::numeric_limits<unsigned int>::max())
      throw std::invalid_argument("Invalid value for '" + key + "': out of range");

    value = static_cast<unsigned int>(parsed);
    return true;
  }

  bool FindDouble(const std::string& json, const std::string& key, double& value)
  {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern))
      return false;

    value = std::stod(match[1].str());
    return true;
  }

  bool FindBool(const std::string& json, const std::string& key, bool& value)
  {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern))
      return false;

    value = match[1].str() == "true";
    return true;
  }

  bool FindString(const std::string& json, const std::string& key, std::string& value)
  {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern))
      return false;

    value = match[1].str();
    return true;
  }

  ILog::TraceLevel ParseTraceLevel(const std::string& value)
  {
    const auto normalized = ToLower(value);
    if (normalized == "debug")
      return ILog::TraceLevel::Debug;
    if (normalized == "verbose")
      return ILog::TraceLevel::Verbose;
    if (normalized == "info")
      return ILog::TraceLevel::Info;
    if (normalized == "warning")
      return ILog::TraceLevel::Warning;
    if (normalized == "error")
      return ILog::TraceLevel::Error;

    throw std::invalid_argument("Invalid log.traceLevel value: " + value);
  }

  Configuration::CallsGenerator::Type ParseGeneratorType(const std::string& value)
  {
    const auto normalized = ToLower(value);
    if (normalized == "random")
      return Configuration::CallsGenerator::Type::Random;
    if (normalized == "fixed")
      return Configuration::CallsGenerator::Type::Fixed;

    throw std::invalid_argument("Invalid callsGenerator.type value: " + value);
  }

  ILog::LogType ParseLogType(const std::string& value)
  {
    const auto normalized = ToLower(value);
    if (normalized == "screen")
      return ILog::LogType::Screen;
    if (normalized == "file")
      return ILog::LogType::File;
    if (normalized == "screenandfile" || normalized == "screen_and_file" || normalized == "both")
      return ILog::LogType::ScreenAndFile;

    throw std::invalid_argument("Invalid log.defaultLogType value: " + value);
  }

  void Validate(const Configuration::Settings& settings)
  {
    if (settings.numberOfElevators == 0)
      throw std::invalid_argument("building.numberOfElevators must be greater than 0");

    if (settings.numberOfElevators > 26)
      throw std::invalid_argument("building.numberOfElevators must be at most 26 (system generates IDs A-Z)");

    if (settings.numberOfFloors < 2)
      throw std::invalid_argument("building.numberOfFloors must be at least 2");

    if (settings.minDelayBetweenCalls > settings.maxDelayBetweenCalls)
      throw std::invalid_argument("callsGenerator.minDelayBetweenCallsMs must be <= maxDelayBetweenCallsMs");

    if (settings.numberOfCalls == 0)
      throw std::invalid_argument("callsGenerator.numberOfCalls must be greater than 0");

    if (settings.averageCallsPerDay == 0)
      throw std::invalid_argument("callsGenerator.averageCallsPerDay must be greater than 0");

    if (settings.maxConcurrentCalls == 0)
      throw std::invalid_argument("callsGenerator.maxConcurrentCalls must be greater than 0");

    if (settings.numberOfSimulationDays == 0)
      throw std::invalid_argument("callsGenerator.numberOfSimulationDays must be greater than 0");

    if (settings.simulationDayDuration.count() == 0)
      throw std::invalid_argument("callsGenerator.simulationDayDurationMs must be greater than 0");

    if (settings.timeToReachTheNextFloor.count() == 0)
      throw std::invalid_argument("elevator.timeToReachTheNextFloorMs must be greater than 0");

    if (!(settings.timeScale > 0.0) || settings.timeScale > 1000.0)
      throw std::invalid_argument("simulation.timeScale must be greater than 0 and at most 1000");

    if (settings.maxPeoplePerElevator == 0)
      throw std::invalid_argument("elevator.maxPeople must be greater than 0");
  }
}

bool Configuration::LoadFromFile(const std::string& path)
{
  const auto json = ReadFile(path);
  if (json.empty())
    throw std::runtime_error("Configuration file '" + path + "' not found or empty");

  Settings settings = g_settings;
  unsigned int value = 0;
  std::string text;

  if (FindUnsigned(json, "numberOfElevators", value))
    settings.numberOfElevators = value;

  if (FindUnsigned(json, "numberOfFloors", value))
    settings.numberOfFloors = value;

  if (FindString(json, "type", text))
    settings.generatorType = ParseGeneratorType(text);

  if (FindUnsigned(json, "numberOfCalls", value))
    settings.numberOfCalls = value;

  if (FindUnsigned(json, "averageCallsPerDay", value))
    settings.averageCallsPerDay = value;

  if (FindUnsigned(json, "maxConcurrentCalls", value))
    settings.maxConcurrentCalls = value;

  if (FindUnsigned(json, "numberOfSimulationDays", value))
    settings.numberOfSimulationDays = value;

  if (FindUnsigned(json, "simulationDayDurationMs", value))
    settings.simulationDayDuration = std::chrono::milliseconds(value);

  if (FindUnsigned(json, "minDelayBetweenCallsMs", value))
    settings.minDelayBetweenCalls = std::chrono::milliseconds(value);

  if (FindUnsigned(json, "maxDelayBetweenCallsMs", value))
    settings.maxDelayBetweenCalls = std::chrono::milliseconds(value);

  if (FindUnsigned(json, "timeToReachTheNextFloorMs", value))
    settings.timeToReachTheNextFloor = std::chrono::milliseconds(value);

  if (FindUnsigned(json, "enterAndExitTimeMs", value))
    settings.enterAndExitTime = std::chrono::milliseconds(value);

  if (FindUnsigned(json, "doorTransitionTimeMs", value))
    settings.doorTransitionTime = std::chrono::milliseconds(value);

  if (FindUnsigned(json, "maxPeople", value))
    settings.maxPeoplePerElevator = value;

  FindDouble(json, "timeScale", settings.timeScale);

  if (FindUnsigned(json, "randomSeed", value))
    settings.randomSeed = value;

  FindBool(json, "autoStop", settings.autoStop);

  if (FindString(json, "traceLevel", text))
    settings.traceLevel = ParseTraceLevel(text);

  if (FindString(json, "defaultLogType", text))
    settings.defaultLogType = ParseLogType(text);

  if (FindString(json, "filePath", text))
    settings.logFilePath = text;

  Validate(settings);
  g_settings = settings;
  return true;
}

const Configuration::Settings& Configuration::Get()
{
  return g_settings;
}

unsigned int Configuration::Building::NumberOfElevators()
{
  return Get().numberOfElevators;
}

unsigned int Configuration::Building::NumberOfFloors()
{
  return Get().numberOfFloors;
}

std::chrono::milliseconds Configuration::Elevator::TimeToReachTheNextFloor()
{
  return Get().timeToReachTheNextFloor;
}

std::chrono::milliseconds Configuration::Elevator::EnterAndExitTime()
{
  return Get().enterAndExitTime;
}

std::chrono::milliseconds Configuration::Elevator::DoorTransitionTime()
{
  return Get().doorTransitionTime;
}

unsigned int Configuration::Elevator::MaxPeople()
{
  return Get().maxPeoplePerElevator;
}

Configuration::CallsGenerator::Type Configuration::CallsGenerator::GeneratorType()
{
  return Get().generatorType;
}

unsigned int Configuration::CallsGenerator::NumberOfCalls()
{
  return Get().numberOfCalls;
}

unsigned int Configuration::CallsGenerator::AverageCallsPerDay()
{
  return Get().averageCallsPerDay;
}

unsigned int Configuration::CallsGenerator::MaxConcurrentCalls()
{
  return Get().maxConcurrentCalls;
}

unsigned int Configuration::CallsGenerator::NumberOfSimulationDays()
{
  return Get().numberOfSimulationDays;
}

long long Configuration::CallsGenerator::SimulationDayDuration()
{
  return Get().simulationDayDuration.count();
}

long long Configuration::CallsGenerator::MinDelayBetweenCalls()
{
  return Get().minDelayBetweenCalls.count();
}

long long Configuration::CallsGenerator::MaxDelayBetweenCalls()
{
  return Get().maxDelayBetweenCalls.count();
}

ILog::TraceLevel Configuration::Log::TraceLevel()
{
  return Get().traceLevel;
}

ILog::LogType Configuration::Log::DefaultLogType()
{
  return Get().defaultLogType;
}

std::string Configuration::Log::FilePath()
{
  return Get().logFilePath;
}

bool Configuration::Log::WriteToScreen()
{
  return Get().defaultLogType == ILog::LogType::Screen ||
    Get().defaultLogType == ILog::LogType::ScreenAndFile;
}

bool Configuration::Log::WriteToFile()
{
  return Get().defaultLogType == ILog::LogType::File ||
    Get().defaultLogType == ILog::LogType::ScreenAndFile;
}

void Configuration::Simulation::StartClock()
{
  g_simulationStartTime = std::chrono::steady_clock::now();
}

double Configuration::Simulation::TimeScale()
{
  return Get().timeScale;
}

unsigned int Configuration::Simulation::RandomSeed()
{
  return Get().randomSeed;
}

bool Configuration::Simulation::AutoStop()
{
  return Get().autoStop;
}

std::chrono::microseconds Configuration::Simulation::ToRealDuration(const std::chrono::milliseconds simulatedDuration)
{
  const auto realMicroseconds = static_cast<double>(simulatedDuration.count()) * 1000.0 / TimeScale();
  return std::chrono::microseconds(static_cast<long long>(realMicroseconds));
}

std::uint64_t Configuration::Simulation::ToSimulatedMs(const std::chrono::microseconds realDuration)
{
  const auto simulatedMs = static_cast<double>(realDuration.count()) / 1000.0 * TimeScale();
  return simulatedMs <= 0.0 ? 0U : static_cast<std::uint64_t>(simulatedMs);
}

void Configuration::Simulation::SleepFor(const std::chrono::milliseconds simulatedDuration)
{
  std::this_thread::sleep_for(ToRealDuration(simulatedDuration));
}

long long Configuration::Simulation::ElapsedMs()
{
  return static_cast<long long>(ToSimulatedMs(std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() - g_simulationStartTime)));
}

double Configuration::Simulation::CurrentHour()
{
  const auto dayDurationMs = static_cast<double>(CallsGenerator::SimulationDayDuration());
  return std::fmod(static_cast<double>(ElapsedMs()), dayDurationMs) / dayDurationMs * 24.0;
}

unsigned int Configuration::Simulation::CurrentDay()
{
  const auto dayDurationMs = CallsGenerator::SimulationDayDuration();
  const auto elapsedDays = static_cast<unsigned int>(ElapsedMs() / dayDurationMs);

  return std::min(elapsedDays + 1U, CallsGenerator::NumberOfSimulationDays());
}

bool Configuration::Simulation::Completed()
{
  const auto targetDurationMs =
    CallsGenerator::SimulationDayDuration() * static_cast<long long>(CallsGenerator::NumberOfSimulationDays());

  return ElapsedMs() >= targetDurationMs;
}

std::string Configuration::Simulation::CurrentDayTimeLabel()
{
  const auto hour = CurrentHour();
  const auto wholeMinutes = static_cast<unsigned int>(hour * 60.0) % (24U * 60U);
  const auto hours = wholeMinutes / 60U;
  const auto minutes = wholeMinutes % 60U;

  std::stringstream label;
  label << "Day " << CurrentDay() << "/" << CallsGenerator::NumberOfSimulationDays()
        << " " << std::setw(2) << std::setfill('0') << hours
        << ":" << std::setw(2) << std::setfill('0') << minutes
        << std::setfill(' ');
  return label.str();
}
