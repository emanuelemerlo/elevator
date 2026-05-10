#include "LogToScreen.h"

#include "ConsoleView.h"

#include <sstream>

LogToScreen::LogToScreen(const std::string& traceId): LogBase(traceId)
{
}

void LogToScreen::LogFunction(const std::shared_ptr<TraceMessage>& message)
{
  if (message->m_level >= m_traceLevelFilter)
  {
    std::stringstream line;
    if (message->m_traceId.empty())
    {
      line << message->m_string;
    }
    else
    {
      line << message->m_traceId << " | " << message->m_string;
    }

    ConsoleView::Instance().WriteLog(line.str());
  }
}

