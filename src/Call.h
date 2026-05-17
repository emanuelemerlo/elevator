/**********************************************************************************
*        File: Call.h
* Description: Implements the people elevator calls.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include "Floors.h"

#include <chrono>
#include <mutex>
#include <utility>
#include <string>

class Call final 
{
public:
  explicit Call(const Floors::FloorNumber startFloor = 0, const Floors::FloorNumber destinationFloor = 0) :
    m_startFloor(startFloor), m_destinationFloor(destinationFloor)
  {
  }

  ~Call() = default;

  Call(const Call&) = delete;
  Call& operator=(const Call&) = delete;

  Call(Call&&) = delete;
  Call& operator=(Call&&) = delete;

public:
  Floors::FloorNumber GetStartFloor() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_startFloor;
  }

  Floors::FloorNumber GetDestinationFloor() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_destinationFloor;
  }

  void SetStartFloor(const Floors::FloorNumber startFloor)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_startFloor = startFloor;
  }

  void SetDestinationFloor(const Floors::FloorNumber destinationFloor)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_destinationFloor = destinationFloor;
  }

  std::string GetAssignedElevator() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_assignedElevator;
  }

  void SetAssignedElevator(std::string assignedElevator)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_assignedElevator = std::move(assignedElevator);
  }

  Direction GetDirection() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_startFloor < m_destinationFloor ? Direction::Up : Direction::Down;
  }

  void MarkQueued()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queuedTime = Clock::now();
  }

  std::chrono::milliseconds GetWaitingTime() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - m_queuedTime);
  }

  bool IsValid() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Floors::IsValid(m_startFloor) && Floors::IsValid(m_destinationFloor) && m_startFloor != m_destinationFloor;
  }

  std::string ToString() const 
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return "[" + m_assignedElevator + " " + std::to_string(m_startFloor) + ", " + std::to_string(m_destinationFloor) + "]";
  }

private:
  typedef std::chrono::steady_clock Clock;

  // Calls are observed by the generator, management, elevator threads and the UI.
  // Protecting the small mutable state avoids races on assignment and wait timing.
  mutable std::mutex m_mutex;

  Floors::FloorNumber m_startFloor = 0;
  Floors::FloorNumber m_destinationFloor = 0;

  std::string m_assignedElevator = "?";
  Clock::time_point m_queuedTime{ Clock::now() };
};

