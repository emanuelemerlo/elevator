#include "Statistics.h"

#include <algorithm>
#include <map>
#include <mutex>

namespace
{
  std::mutex g_mutex;
  StatisticsSnapshot g_statistics;
  std::map<std::string, std::uint64_t> g_elevatorAssignments;

  void RefreshElevatorAssignments()
  {
    // Keep the public snapshot compact while allowing cheap incremental updates
    // to the per-elevator assignment counters during the simulation.
    g_statistics.elevatorAssignments.clear();
    for (const auto& assignment : g_elevatorAssignments)
      g_statistics.elevatorAssignments.push_back({ assignment.first, assignment.second });
  }
}

double StatisticsSnapshot::AverageWaitTimeMs() const
{
  if (boardedPassengers == 0U)
    return 0.0;

  return static_cast<double>(totalWaitTimeMs) / static_cast<double>(boardedPassengers);
}

double StatisticsSnapshot::AverageAssignmentTimeMs() const
{
  if (assignmentDecisions == 0U)
    return 0.0;

  return static_cast<double>(totalAssignmentTimeUs) / static_cast<double>(assignmentDecisions) / 1000.0;
}

void Statistics::Reset()
{
  std::lock_guard<std::mutex> lock(g_mutex);
  g_statistics = StatisticsSnapshot();
  g_elevatorAssignments.clear();
}

void Statistics::RecordQueuedCall()
{
  std::lock_guard<std::mutex> lock(g_mutex);
  ++g_statistics.queuedCalls;
}

void Statistics::RecordAssignedCall(const std::string& elevatorId)
{
  std::lock_guard<std::mutex> lock(g_mutex);
  ++g_statistics.assignedCalls;
  ++g_elevatorAssignments[elevatorId];
}

void Statistics::RecordAssignmentDecision(
  const std::chrono::microseconds assignmentTime,
  const bool forced,
  const bool availableElevatorFound)
{
  std::lock_guard<std::mutex> lock(g_mutex);

  // Dispatch timings are measured around the whole assignment critical section,
  // so spikes include both scoring work and contention on concurrent call bursts.
  ++g_statistics.assignmentDecisions;

  if (forced)
    ++g_statistics.forcedAssignments;

  if (!availableElevatorFound)
    ++g_statistics.noAvailableElevatorDecisions;

  const auto assignmentTimeUs = static_cast<std::uint64_t>(assignmentTime.count());
  g_statistics.totalAssignmentTimeUs += assignmentTimeUs;
  g_statistics.maxAssignmentTimeUs = std::max(g_statistics.maxAssignmentTimeUs, assignmentTimeUs);
}

void Statistics::RecordBoardedPassenger(const std::chrono::milliseconds waitTime)
{
  std::lock_guard<std::mutex> lock(g_mutex);
  ++g_statistics.boardedPassengers;
  const auto waitTimeMs = static_cast<std::uint64_t>(waitTime.count());
  g_statistics.totalWaitTimeMs += waitTimeMs;
  g_statistics.maxWaitTimeMs = std::max(g_statistics.maxWaitTimeMs, waitTimeMs);
}

void Statistics::RecordCompletedPassenger()
{
  std::lock_guard<std::mutex> lock(g_mutex);
  ++g_statistics.completedPassengers;
}

StatisticsSnapshot Statistics::GetSnapshot()
{
  std::lock_guard<std::mutex> lock(g_mutex);
  RefreshElevatorAssignments();
  return g_statistics;
}
