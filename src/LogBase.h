/**********************************************************************************
*        File: LogBase.h
* Description: Implements the log service basic logic.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include <deque>
#include <atomic>
#include <thread>

#include "ILog.h"
#include "WorkerThread.h"

/**
 * \brief Implements the log basic producer/consumer logic with a message queue and a trace thread.
 */
class LogBase : public ILog
{
private:
  class TraceThread final : public WorkerThread<LogBase>
  {
  public:
    explicit TraceThread(LogBase* logBase = nullptr) : WorkerThread<LogBase>(logBase) {}

  protected:
    void CycleFunction(LogBase* logBase) override;
  };

protected:
  struct TraceMessage
  {
    typedef std::chrono::steady_clock Clock;
    typedef std::chrono::time_point<Clock> TimeStamp;

    TraceMessage(
      const std::string& message,
      const std::string& traceId,
      const TraceLevel level,
      const std::thread::id& threadId,
      const TimeStamp& timeStamp = Clock::now())
    {
      m_string = message;
      m_traceId = traceId;
      m_level = level;
      m_timeStamp = timeStamp;
      m_threadId = threadId;
    }

    std::string m_string;
    std::string m_traceId;
    TraceLevel m_level = TraceLevel::Info;
    TimeStamp m_timeStamp;
    std::thread::id m_threadId;
  };

public:
  explicit LogBase(std::string traceId = "");
  virtual ~LogBase();

  LogBase(const LogBase&) = delete;
  LogBase(LogBase&&) = delete;

  LogBase& operator=(const LogBase&) = delete;
  LogBase& operator=(LogBase&&) = delete;

public: // ILog 
  void Trace(const std::stringstream& message, const TraceLevel level = TraceLevel::Info, const std::string& messageSpecificId = "") override;
  void Trace(const std::string& message, const TraceLevel level = TraceLevel::Info, const std::string& messageSpecificId = "") override;

  void SetTraceId(const std::string& traceId) override { m_traceId = traceId; }
  const std::string& GetTraceId() const override { return m_traceId; }

  void SetTraceLevelFilter(const TraceLevel traceLevelThreshold) override { m_traceLevelFilter = traceLevelThreshold; }

private:
  void Enqueue(const std::shared_ptr<TraceMessage>& traceMessage);

  virtual void LogFunction(const std::shared_ptr<TraceMessage>& message) = 0;

  std::unique_ptr<TraceThread>& GetThreadInstance();

  static void AddRef() { ++m_refCount; }
  static void Release() { --m_refCount; }

private:
  static std::deque<std::shared_ptr<TraceMessage>> m_messageQueue;
  static std::unique_ptr<std::mutex> m_messageQueueMutex;

  static std::atomic_uint m_refCount;

protected:
  std::string m_traceId;

  static TraceLevel m_traceLevelFilter;
};

