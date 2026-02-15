#include "LogToScreen.h"

#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace
{
  std::mutex g_screenMutex;

  const char* ToString(const ILog::TraceLevel level)
  {
    switch (level)
    {
    case ILog::TraceLevel::Debug: return "DBG";
    case ILog::TraceLevel::Verbose: return "VRB";
    case ILog::TraceLevel::Info: return "INF";
    case ILog::TraceLevel::Warning: return "WRN";
    case ILog::TraceLevel::Error: return "ERR";
    default: return "UNK";
    }
  }
}

LogToScreen::LogToScreen(const std::string& traceId): LogBase(traceId)
{
}

void LogToScreen::LogFunction(const std::shared_ptr<TraceMessage>& message)
{
  if (message->m_level < m_traceLevelFilter)
    return;

  static const auto logStart = TraceMessage::Clock::now();
  const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(message->m_timeStamp - logStart).count();

  std::stringstream line;
  line << "[+" << std::setw(6) << elapsedMs << "ms]";
  line << "[" << ToString(message->m_level) << "]";
  line << "[T:" << message->m_threadId << "] ";

  if (!message->m_traceId.empty())
    line << message->m_traceId << " | ";

  line << message->m_string;

  std::lock_guard<std::mutex> lock(g_screenMutex);
  std::cout << line.str() << std::endl;
}

