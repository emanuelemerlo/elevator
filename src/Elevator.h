/**********************************************************************************
*        File: Elevator.h
* Description: Implements the elevator logic.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>

#include "Floors.h"
#include "People.h"

using namespace std::chrono_literals;

enum class ElevatorStatus
{
  OutOfOrder = -1,
  Idle = 0,
  PeopleEnterAndExit,
  Moving,
};

enum class DoorsStatus
{
  Open,
  Closed
};

struct ElevatorSnapshot
{
  std::string id;
  std::string name;
  Floors::FloorNumber currentFloor;
  ElevatorStatus status;
  Direction direction;
  std::size_t peopleCount;
};

class Elevator final
{
public:
  explicit Elevator(const std::string& id = "");

  Elevator(const Elevator&) = delete;
  Elevator(Elevator&&) = delete;

  ~Elevator();

  Elevator& operator=(const Elevator&) = delete;
  Elevator& operator=(Elevator&& other) noexcept = delete;

public:
  bool Available(const std::shared_ptr<Call>& call) const;
  bool AnswerToCall(const std::shared_ptr<Call>& call);

  void ShutDown();

  void SetId(std::string id);
  std::string GetId() const { return m_elevatorId; }

  std::string GetElevatorName() const { return m_name; }

  Floors::FloorNumber GetCurrentFloor() const { return m_currentFloor.load(); }
  ElevatorStatus GetStatus() const { return m_status.load(); }
  Direction GetDirection() const { return m_currentDirection.load(); }
  ElevatorSnapshot GetSnapshot() const;
  std::size_t GetPeopleCount() const { return m_people.Count(); }
  std::size_t GetCommittedPeopleCount() const;
  std::size_t GetQueuedStopsCount() const { return m_floors.CountStops(); }
  std::size_t GetQueuedStopsBetween(Floors::FloorNumber firstFloor, Floors::FloorNumber lastFloor) const
    { return m_floors.CountStopsBetween(firstFloor, lastFloor); }
  std::uint64_t GetTotalFloorsTravelled() const { return m_totalFloorsTravelled.load(); }
  std::uint64_t GetRunCounter() const { return m_runCounter.load(); }
  std::uint64_t GetDoorCycles() const { return m_doorCycles.load(); }

private:
  bool OpenDoors();
  bool CloseDoors();

  void PeopleEnterAndExit();
  void Move(Floors::FloorNumber requestedFloor);
  void Stop();

  void RestoreDestinationStops();

private:
  void ElevatorThreadFunction();

private:
  std::atomic<Floors::FloorNumber> m_currentFloor{ 0 };
  Floors m_floors;

  People m_people;

  std::atomic<ElevatorStatus> m_status{ ElevatorStatus::Idle };
  std::atomic<Direction> m_currentDirection{ Direction::None };
  std::atomic<Direction> m_displayDirection{ Direction::None };

  std::atomic<DoorsStatus> m_doorsStatus{ DoorsStatus::Closed };

  std::condition_variable m_go;
  std::mutex m_goMutex;

  std::string m_elevatorId = "?";
  std::string m_name;

  std::unique_ptr<std::thread> m_thread;
  std::atomic_bool m_shutdownRequested{ false };
  std::atomic_bool m_working{ false };

  std::atomic<std::uint64_t> m_totalFloorsTravelled{ 0U };
  std::atomic<std::uint64_t> m_runCounter{ 0U };
  std::atomic<std::uint64_t> m_doorCycles{ 0U };

  Log m_log;
};

