#include "Log.h"
#include "LogToScreen.h"

Log::Log(const std::string& traceId, const LogType logType)
{
  m_logType = logType;
  GetLog(traceId);
}

std::shared_ptr<ILog>& Log::GetLog(const std::string& traceId) 
{
  if (m_implementation == nullptr)
  {
    switch(m_logType)
    {
    case LogType::Screen:
    case LogType::File:
    case LogType::ScreenAndFile:
      m_implementation = std::make_shared<LogToScreen>(traceId);
      m_implementation->SetTraceLevelFilter(Configuration::Log::TraceLevel());
      break;

    default:
      throw std::invalid_argument("Invalid log type");
    }    
  }

  return m_implementation;
}

void Log::Trace(const std::stringstream& message, const TraceLevel level, const std::string& messageSpecificId) 
{
  if (m_implementation != nullptr)
    m_implementation->Trace(message, level, messageSpecificId);
}

void Log::Trace(const std::string& message, const TraceLevel level, const std::string& messageSpecificId) 
{
  if (m_implementation != nullptr)
    m_implementation->Trace(message, level, messageSpecificId);
}

void Log::SetTraceId(const std::string& traceId)
{
  if (m_implementation != nullptr)
    m_implementation->SetTraceId(traceId);
}

const std::string& Log::GetTraceId() const
{
  if (m_implementation == nullptr)
    throw std::invalid_argument("Invalid pointer to implementation");

  return m_implementation->GetTraceId();
}

void Log::SetTraceLevelFilter(const TraceLevel startLevel)
{
  if (m_implementation == nullptr)
    throw std::invalid_argument("Invalid pointer to implementation");

  m_implementation->SetTraceLevelFilter(startLevel);
}
