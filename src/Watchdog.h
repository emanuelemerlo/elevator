/**********************************************************************************
*        File: Watchdog.h
* Description: Implements a simple timer which call the passed function if expires.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <string>
#include <utility>

/**
 * \brief Implements a simple timer which call the passed function if expires.
 */
class Watchdog final
{
public:
  /**
   * \brief Default constructor.
   * \param id Id of the watchdog.
   */
  explicit Watchdog(std::string id) : m_id(std::move(id))
  {
  }

  /**
   * \brief Construct and start the watchdog.
   * \param id Id of the watchdog.
   * \param timeout Milliseconds before the timer expires.
   * \param timeoutCallback Function called if the timer expires.
   */
  Watchdog(
    std::string id,
    const std::chrono::milliseconds timeout,
    const std::function<void(const std::string&)>& timeoutCallback) : m_id(std::move(id))
  {
    Start(timeout, timeoutCallback);
  }

  ~Watchdog()
  {
    Stop();
  }

  Watchdog(const Watchdog&) = delete;
  Watchdog& operator=(const Watchdog&) = delete;

  Watchdog(Watchdog&&) = delete;
  Watchdog& operator=(Watchdog&&) = delete;

public:
  /**
   * \brief Start the watchdog. If is already running will be restarted.
   * \param timeout Milliseconds before the timer expires.
   * \param timeoutCallback Function called if the timer expires.
   */
  void Start(
    const std::chrono::milliseconds timeout,
    const std::function<void(const std::string&)>& timeoutCallback)
  {
    if (m_started)
      Stop(); // already running: stop and restart

    m_stopped = false; // Stop() leaves the flag set: without this a restarted watchdog would expire immediately

    m_guardFunctionResult = std::async(
      std::launch::async,
      bind(&Watchdog::GuardFunction, this, timeout, timeoutCallback));

    m_started = true;
  }

  /**
   * \brief Stop the watchdog.
   * \return True if the watchdog has been correctly stopped.
   */
  void Stop()
  {
    if (!m_started)
      return; // not started

    {
      // Set the flag under the mutex used by the waiting thread, otherwise the
      // notification can be lost and the callback fires even if Stop() was called.
      std::lock_guard<std::mutex> lock(m_mutex);
      m_stopped = true;
    }

    m_condition.notify_all();

    m_guardFunctionResult.wait();

    m_started = false;
  }

  bool IsStopped() const { return m_stopped; }

private:
  bool GuardFunction(
    const std::chrono::milliseconds timeout,
    const std::function<void(const std::string&)>& timeoutCallback)
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!m_condition.wait_for(lock, timeout, [this]()-> bool { return this->m_stopped; }))
    {
      lock.unlock(); // do not hold the mutex while running user code (Stop() would block on it)
      timeoutCallback(m_id);
    }

    return m_stopped;
  }

private:
  std::string m_id;

  std::mutex m_mutex;
  std::condition_variable m_condition;

  std::atomic_bool m_started{ false };
  std::atomic_bool m_stopped{ false };

  std::future<bool> m_guardFunctionResult;
};


