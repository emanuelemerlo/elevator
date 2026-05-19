#include "LogToScreen.h"

#include "Configuration.h"
#include "ConsoleView.h"
#include "LogFileSink.h"

#include <sstream>

LogToScreen::LogToScreen(const std::string& traceId): LogBase(traceId)
{
}

void LogToScreen::LogFunction(const std::shared_ptr<TraceMessage>& message)
{
  if (message->m_level >= m_traceLevelFilter)
  {
    std::stringstream formattedLine;
    if (message->m_traceId.empty())
      formattedLine << message->m_string;
    else
      formattedLine << message->m_traceId << " | " << message->m_string;

    const auto line = formattedLine.str();

    if (Configuration::Log::WriteToScreen())
      ConsoleView::Instance().WriteLog(line);

    if (Configuration::Log::WriteToFile())
      LogFileSink::WriteLine(line);
  }
}

