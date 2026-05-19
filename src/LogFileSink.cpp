#include "LogFileSink.h"

#include "Configuration.h"

#include <fstream>
#include <mutex>

namespace
{
  std::mutex g_fileMutex;
  bool g_fileInitialized = false;

  std::ofstream OpenFile()
  {
    const auto mode = g_fileInitialized ? std::ios::app : std::ios::trunc;
    std::ofstream file(Configuration::Log::FilePath(), std::ios::out | mode);
    if (file.is_open())
      g_fileInitialized = true;

    return file;
  }
}

void LogFileSink::WriteLine(const std::string& line)
{
  std::lock_guard<std::mutex> lock(g_fileMutex);

  auto file = OpenFile();
  if (!file.is_open())
    return;

  file << line << '\n';
}

void LogFileSink::WriteLines(const std::vector<std::string>& lines)
{
  std::lock_guard<std::mutex> lock(g_fileMutex);

  auto file = OpenFile();
  if (!file.is_open())
    return;

  for (const auto& line : lines)
    file << line << '\n';
}
