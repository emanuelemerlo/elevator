#include "LogBase.h"

#include "Watchdog.h"
#include "Configuration.h"

#include <sstream>
#include <thread>
#include <utility>

std::deque<std::shared_ptr<LogBase::TraceMessage>> LogBase::m_messageQueue;
std::unique_ptr<std::mutex> LogBase::m_messageQueueMutex{ std::make_unique<std::mutex>() };

LogBase::TraceLevel LogBase::m_traceLevelFilter{ Configuration::Log::TraceLevel() };

std::atomic_uint LogBase::m_refCount;

LogBase::LogBase(std::string traceId) : m_traceId(std::move(traceId))
{
  AddRef();
  GetThreadInstance()->Start();
}

LogBase::~LogBase()
{
  Release();

  if(m_refCount == 0)
    GetThreadInstance()->Stop();
}

void LogBase::Trace(const std::stringstream& message, const TraceLevel level, const std::string& messageSpecificId) 
{
  Enqueue(std::make_shared<TraceMessage>(message.str(), messageSpecificId.empty() ? m_traceId : messageSpecificId, level));
}

void LogBase::Trace(const std::string& message, const TraceLevel level, const std::string& messageSpecificId) 
{
  Enqueue(std::make_shared<TraceMessage>(message, messageSpecificId.empty() ? m_traceId : messageSpecificId, level));
}


void LogBase::Enqueue(const std::shared_ptr<TraceMessage>& traceMessage) 
{
  if (m_messageQueueMutex == nullptr) // Preconditions
    return;
    
  std::lock_guard<std::mutex> lockMessageQueue(*m_messageQueueMutex);
  m_messageQueue.push_back(traceMessage);

  GetThreadInstance()->Go();
}

void LogBase::FlushPendingMessages()
{
  for (;;)
  {
    {
      std::lock_guard<std::mutex> lockMessageQueue(*m_messageQueueMutex);
      if (m_messageQueue.empty())
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

std::unique_ptr<LogBase::TraceThread>& LogBase::GetThreadInstance()
{
  static auto thread = std::make_unique<TraceThread>(this);
  return thread;
}

void LogBase::TraceThread::CycleFunction(LogBase* logBase)
{
  while (!m_messageQueue.empty() && !StopRequested())
  {
    std::lock_guard<std::mutex> lockMessageQueue(*m_messageQueueMutex);

    const auto message = std::move(m_messageQueue.front());
    m_messageQueue.pop_front();

    if(logBase != nullptr)
      logBase->LogFunction(message);
  }
}
