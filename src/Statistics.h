/**********************************************************************************
*        File: Statistics.h
* Description: Collects simulation statistics.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct ElevatorAssignmentStatistics
{
  std::string elevatorId;
  std::uint64_t assignedCalls{ 0U };
};

struct StatisticsSnapshot
{
  std::uint64_t queuedCalls{ 0U };
  std::uint64_t assignedCalls{ 0U };
  std::uint64_t boardedPassengers{ 0U };
  std::uint64_t completedPassengers{ 0U };
  std::uint64_t totalWaitTimeMs{ 0U };
  std::uint64_t maxWaitTimeMs{ 0U };
  std::uint64_t assignmentDecisions{ 0U };
  std::uint64_t forcedAssignments{ 0U };
  std::uint64_t noAvailableElevatorDecisions{ 0U };
  std::uint64_t capacityLimitedDecisions{ 0U };
  std::uint64_t maxCabinPeople{ 0U };
  std::uint64_t maxCommittedElevatorLoad{ 0U };
  std::uint64_t totalAssignmentTimeUs{ 0U };
  std::uint64_t maxAssignmentTimeUs{ 0U };
  std::vector<ElevatorAssignmentStatistics> elevatorAssignments;

  double AverageWaitTimeMs() const;
  double AverageAssignmentTimeMs() const;
};

namespace Statistics
{
  void Reset();
  void RecordQueuedCall();
  void RecordAssignedCall(const std::string& elevatorId);
  void RecordAssignmentDecision(std::chrono::microseconds assignmentTime, bool forced, bool availableElevatorFound);
  void RecordCapacityLimitedDecision();
  void RecordCabinPeopleCount(std::size_t cabinPeopleCount);
  void RecordCommittedElevatorLoad(std::size_t committedLoad);
  void RecordBoardedPassenger(std::chrono::milliseconds waitTime);
  void RecordCompletedPassenger();
  StatisticsSnapshot GetSnapshot();
}
