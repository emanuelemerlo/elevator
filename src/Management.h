/**********************************************************************************
*        File: Management.h
* Description: Elevator(s) management, manage the calls assignation. 
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include "Log.h"

#include <vector>
#include <memory>
#include <mutex>

#include "Elevator.h"

class Management final 
{
public:
  explicit Management(const unsigned int numberOfElevators);

  Management(const Management&) = delete;
  Management(Management&&) = delete;

  ~Management();
  
  Management& operator=(const Management&) = delete;
  Management& operator=(Management&&) = delete;

public:
  bool AssignCall(const class std::shared_ptr<class Call>& call);

  void Shutdown();
  void TraceStatistics();

  std::vector<ElevatorSnapshot> GetElevatorSnapshots() const;

private:
  std::vector<std::unique_ptr<class Elevator>> m_elevators;
  std::mutex m_assignMutex;
  bool m_shutdownCompleted{ false };

  Log m_log;
};
