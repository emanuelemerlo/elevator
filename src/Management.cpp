#include "Management.h"

#include "Elevator.h"
#include "Configuration.h"
#include "ConsoleView.h"
#include "LogBase.h"
#include "LogFileSink.h"
#include "Statistics.h"

#include <random>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <string>
#include <thread>
#include <vector>

namespace
{
  struct AssignmentCandidate
  {
    Elevator* elevator{ nullptr };
    double score{ std::numeric_limits<double>::max() };
    bool available{ false };
    bool capacityLimited{ false };
    std::size_t committedLoad{ 0U };
  };

  bool IsMoving(const ElevatorStatus status)
  {
    return status == ElevatorStatus::Moving || status == ElevatorStatus::PeopleEnterAndExit;
  }

  bool IsOnTheWay(const Direction direction, const Floors::FloorNumber currentFloor, const Floors::FloorNumber callFloor)
  {
    if (direction == Direction::Up)
      return callFloor >= currentFloor;

    if (direction == Direction::Down)
      return callFloor <= currentFloor;

    return true;
  }

  Floors::FloorNumber EstimateRouteFloors(
    const Elevator& elevator,
    const std::shared_ptr<Call>& call,
    const bool directionCompatible,
    const bool callStillAhead)
  {
    const auto currentFloor = elevator.GetCurrentFloor();
    const auto callFloor = call->GetStartFloor();
    const auto currentDirection = elevator.GetDirection();

    if (!IsMoving(elevator.GetStatus()) || currentDirection == Direction::None)
      return static_cast<Floors::FloorNumber>(std::abs(static_cast<int>(currentFloor) - static_cast<int>(callFloor)));

    if (directionCompatible && callStillAhead)
      return static_cast<Floors::FloorNumber>(std::abs(static_cast<int>(currentFloor) - static_cast<int>(callFloor)));

    // If the car is moving away from the caller, estimate the cost of finishing
    // the current sweep before coming back. This keeps opposite-direction pickups
    // possible, but makes them less attractive than on-the-way pickups.
    if (currentDirection == Direction::Up)
      return (Floors::TopFloor() - currentFloor) + (Floors::TopFloor() - callFloor);

    if (currentDirection == Direction::Down)
      return (currentFloor - Floors::BottomFloor) + (callFloor - Floors::BottomFloor);

    return static_cast<Floors::FloorNumber>(std::abs(static_cast<int>(currentFloor) - static_cast<int>(callFloor)));
  }

  std::size_t EstimateIntermediateStops(
    const Elevator& elevator,
    const std::shared_ptr<Call>& call,
    const bool directionCompatible,
    const bool callStillAhead)
  {
    if (directionCompatible && callStillAhead)
      return elevator.GetQueuedStopsBetween(elevator.GetCurrentFloor(), call->GetStartFloor());

    return elevator.GetQueuedStopsCount();
  }

  double WearScore(const Elevator& elevator)
  {
    // Wear is deliberately a small additive score: waiting time should dominate,
    // but long-term ties should drift toward less-used cars.
    return
      static_cast<double>(elevator.GetTotalFloorsTravelled()) * 0.75 +
      static_cast<double>(elevator.GetRunCounter()) * 8.0 +
      static_cast<double>(elevator.GetDoorCycles()) * 12.0;
  }

  AssignmentCandidate EvaluateElevator(Elevator& elevator, const std::shared_ptr<Call>& call)
  {
    AssignmentCandidate candidate;
    candidate.elevator = &elevator;

    if (!call->IsValid() || elevator.GetStatus() == ElevatorStatus::OutOfOrder)
      return candidate;

    const auto maxPeople = Configuration::Elevator::MaxPeople();
    const auto committedLoad = elevator.GetCommittedPeopleCount();
    candidate.committedLoad = committedLoad;
    candidate.capacityLimited = committedLoad >= maxPeople;
    Statistics::RecordCommittedElevatorLoad(committedLoad);

    const auto loadRatio = static_cast<double>(committedLoad) / static_cast<double>(maxPeople);

    if (candidate.capacityLimited)
      return candidate;

    const auto currentDirection = elevator.GetDirection();
    const auto callDirection = call->GetDirection();
    const auto currentFloor = elevator.GetCurrentFloor();
    const auto callFloor = call->GetStartFloor();

    const auto idle = elevator.GetStatus() == ElevatorStatus::Idle || currentDirection == Direction::None;
    const auto directionCompatible = idle || currentDirection == callDirection;
    const auto callStillAhead = IsOnTheWay(currentDirection, currentFloor, callFloor);
    const auto onTheWay = directionCompatible && callStillAhead;

    const auto routeFloors = EstimateRouteFloors(elevator, call, directionCompatible, callStillAhead);
    const auto intermediateStops = EstimateIntermediateStops(elevator, call, directionCompatible, callStillAhead);

    const auto floorMs = static_cast<double>(Configuration::Elevator::TimeToReachTheNextFloor().count());
    const auto stopMs = static_cast<double>(Configuration::Elevator::EnterAndExitTime().count());

    // Score is expressed in time-like units. Travel and scheduled stops form the
    // base ETA, then penalties account for state, capacity and mechanical wear.
    double score = routeFloors * floorMs;
    score += intermediateStops * stopMs;

    if (elevator.GetStatus() == ElevatorStatus::PeopleEnterAndExit)
      score += stopMs;

    if (!idle && !onTheWay)
      score += directionCompatible ? floorMs * 2.0 : floorMs * 4.0;

    if (loadRatio >= 0.80)
      score += stopMs * 6.0 * loadRatio;
    else
      score += stopMs * loadRatio;

    score += WearScore(elevator);

    candidate.score = score;
    candidate.available = elevator.Available(call);
    return candidate;
  }

  std::string Separator(const std::size_t width)
  {
    return std::string(width, '-');
  }

  std::string FormatSeconds(const double milliseconds)
  {
    std::stringstream value;
    value << std::fixed << std::setprecision(2) << milliseconds / 1000.0 << " s";
    return value.str();
  }

  std::string FormatMilliseconds(const double milliseconds)
  {
    std::stringstream value;
    value << std::fixed << std::setprecision(3) << milliseconds << " ms";
    return value.str();
  }

  std::string FormatRatio(const double ratio)
  {
    std::stringstream value;
    value << std::fixed << std::setprecision(2) << ratio << "x";
    return value.str();
  }

  std::string FormatPercent(const double ratio)
  {
    std::stringstream value;
    value << std::fixed << std::setprecision(1) << ratio * 100.0 << "%";
    return value.str();
  }

  std::uint64_t AssignedCallsForElevator(
    const StatisticsSnapshot& statistics,
    const std::string& elevatorId)
  {
    const auto assignment = std::find_if(
      statistics.elevatorAssignments.begin(),
      statistics.elevatorAssignments.end(),
      [&elevatorId](const ElevatorAssignmentStatistics& item) { return item.elevatorId == elevatorId; });

    return assignment == statistics.elevatorAssignments.end() ? 0U : assignment->assignedCalls;
  }

  std::uint64_t CountUnassignedElevators(
    const StatisticsSnapshot& statistics,
    const std::vector<std::unique_ptr<Elevator>>& elevators)
  {
    return static_cast<std::uint64_t>(std::count_if(
      elevators.begin(),
      elevators.end(),
      [&statistics](const std::unique_ptr<Elevator>& elevator)
      {
        return AssignedCallsForElevator(statistics, elevator->GetId()) == 0U;
      }));
  }

  struct FleetSizingRecommendation
  {
    unsigned int recommendedElevators{ 1U };
    std::string signal;
    std::string reason;
    double targetAverageWaitMs{ 0.0 };
    double forcedAssignmentRatio{ 0.0 };
    double noAvailableRatio{ 0.0 };
    std::uint64_t unassignedElevators{ 0U };
  };

  struct CabinSizingRecommendation
  {
    unsigned int recommendedMaxPeople{ 1U };
    std::string signal;
    std::string reason;
  };

  CabinSizingRecommendation BuildCabinSizingRecommendation(const StatisticsSnapshot& statistics)
  {
    CabinSizingRecommendation recommendation;
    const auto configuredMaxPeople = Configuration::Elevator::MaxPeople();
    const auto observedPeak = std::max(statistics.maxCabinPeople, statistics.maxCommittedElevatorLoad);

    recommendation.recommendedMaxPeople = std::max(1U, configuredMaxPeople);

    if (statistics.capacityLimitedDecisions > 0U)
    {
      recommendation.recommendedMaxPeople = std::max<unsigned int>(
        configuredMaxPeople + 1U,
        static_cast<unsigned int>(observedPeak + 1U));
      recommendation.signal = "Increase";
      recommendation.reason = "cabins reached full load";
    }
    else if (observedPeak > 0U && observedPeak < configuredMaxPeople * 0.5 && configuredMaxPeople > 1U)
    {
      recommendation.recommendedMaxPeople = std::max<unsigned int>(1U, static_cast<unsigned int>(observedPeak + 1U));
      recommendation.signal = "Can reduce";
      recommendation.reason = "observed load stayed low";
    }
    else
    {
      recommendation.signal = "Keep";
      recommendation.reason = "cabin capacity looks balanced";
    }

    return recommendation;
  }

  FleetSizingRecommendation BuildFleetSizingRecommendation(
    const StatisticsSnapshot& statistics,
    const std::vector<std::unique_ptr<Elevator>>& elevators,
    const std::uint64_t pendingBoarding)
  {
    FleetSizingRecommendation recommendation;
    const auto currentElevators = static_cast<unsigned int>(elevators.size());
    recommendation.recommendedElevators = std::max(1U, currentElevators);

    const auto assignmentDecisions = static_cast<double>(std::max<std::uint64_t>(1U, statistics.assignmentDecisions));
    recommendation.forcedAssignmentRatio = static_cast<double>(statistics.forcedAssignments) / assignmentDecisions;
    recommendation.noAvailableRatio = static_cast<double>(statistics.noAvailableElevatorDecisions) / assignmentDecisions;
    recommendation.unassignedElevators = CountUnassignedElevators(statistics, elevators);

    const auto travelMs = static_cast<double>(Configuration::Elevator::TimeToReachTheNextFloor().count());
    const auto stopMs = static_cast<double>(Configuration::Elevator::EnterAndExitTime().count());
    recommendation.targetAverageWaitMs = std::max(5000.0, travelMs * 2.0 + stopMs);

    unsigned int pressureScore = 0U;
    if (pendingBoarding > 0U)
      pressureScore += 2U;
    if (statistics.AverageWaitTimeMs() > recommendation.targetAverageWaitMs)
      ++pressureScore;
    if (statistics.maxWaitTimeMs > static_cast<std::uint64_t>(recommendation.targetAverageWaitMs * 3.0))
      ++pressureScore;
    if (recommendation.forcedAssignmentRatio > 0.05)
      ++pressureScore;
    if (recommendation.forcedAssignmentRatio > 0.15)
      ++pressureScore;
    if (recommendation.noAvailableRatio > 0.05)
      ++pressureScore;

    const auto canConsiderReduction =
      currentElevators > 1U &&
      pendingBoarding == 0U &&
      statistics.forcedAssignments == 0U &&
      statistics.noAvailableElevatorDecisions == 0U &&
      statistics.AverageWaitTimeMs() < recommendation.targetAverageWaitMs * 0.5 &&
      recommendation.unassignedElevators > 0U;

    // The recommendation is intentionally conservative: one simulation run is a
    // sample, not a capacity study. Increase on sustained pressure, reduce only
    // when service is comfortable and at least one elevator did no useful work.
    if (pressureScore >= 5U)
    {
      recommendation.recommendedElevators = currentElevators + 2U;
      recommendation.signal = "Increase strongly";
      recommendation.reason = "high wait/backlog pressure";
    }
    else if (pressureScore >= 3U)
    {
      recommendation.recommendedElevators = currentElevators + 1U;
      recommendation.signal = "Increase";
      recommendation.reason = "service pressure observed";
    }
    else if (canConsiderReduction)
    {
      recommendation.recommendedElevators = currentElevators - 1U;
      recommendation.signal = "Can reduce";
      recommendation.reason = "spare elevator capacity";
    }
    else
    {
      recommendation.signal = "Keep";
      recommendation.reason = "capacity looks balanced";
    }

    return recommendation;
  }

  void WriteFinalReportToFile(const std::vector<std::string>& report)
  {
    LogFileSink::WriteLine("");
    LogFileSink::WriteLines(report);
  }
}

Management::Management(const unsigned int numberOfElevators)
{
  for(auto elevatorIndex = 0U; elevatorIndex < numberOfElevators; ++elevatorIndex)
  {
    const auto elevatorId = std::string(1U, static_cast<char>('A' + elevatorIndex));
    m_elevators.push_back(std::make_unique<Elevator>(elevatorId));
  }

  m_log.SetTraceId("Management");
}

Management::~Management()
{
  Shutdown();
}

void Management::Shutdown()
{
  if (m_shutdownCompleted || m_elevators.empty())
    return;

  m_log.Trace("Shutdown in progress...", Log::TraceLevel::Verbose);

  for (auto& elevator : m_elevators)
    elevator->ShutDown();

  m_log.Trace("Shutdown completed", Log::TraceLevel::Verbose);
  m_shutdownCompleted = true;
}

bool Management::AssignCall(const std::shared_ptr<Call>& call)
{
  const auto assignmentStartTime = std::chrono::steady_clock::now();

  // Assignment is serialized because calls are generated asynchronously and each
  // decision reads mutable elevator queues and writes the assigned elevator id.
  std::lock_guard<std::mutex> lock(m_assignMutex);

  bool callAssigned = false;
  bool forcedAssignment = false;
  bool availableElevatorFound = false;

  const auto assignCall = [&call, this](const auto& elevator)
  {
    std::stringstream message;
    message << "Call " << call->ToString() << " assigned to elevator: " << elevator->GetId();
    m_log.Trace(message);

    call->SetAssignedElevator(elevator->GetId());
    Statistics::RecordCommittedElevatorLoad(elevator->GetCommittedPeopleCount());
    const auto answered = elevator->AnswerToCall(call);

    if (answered)
      Statistics::RecordAssignedCall(elevator->GetId());

    return answered;
  };

  std::vector<AssignmentCandidate> candidates;
  candidates.reserve(m_elevators.size());

  do
  {
    candidates.clear();

    for (auto& elevator : m_elevators)
      candidates.push_back(EvaluateElevator(*elevator, call));

    std::sort(candidates.begin(), candidates.end(),
      [](const AssignmentCandidate& a, const AssignmentCandidate& b) { return a.score < b.score; });

    const auto allCandidatesCapacityLimited = std::all_of(candidates.begin(), candidates.end(),
      [](const AssignmentCandidate& candidate) { return candidate.capacityLimited; });

    if (!allCandidatesCapacityLimited)
      break;

    Statistics::RecordCapacityLimitedDecision();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (true);

  // Availability is stricter than scoring. The metric helps detect moments where
  // the dispatcher had to fall back because every ideal candidate was busy/full.
  availableElevatorFound = std::any_of(candidates.begin(), candidates.end(),
    [](const AssignmentCandidate& candidate) { return candidate.available; });

  for(auto& candidate : candidates)
  {
    if (m_elevators.size() == 1 || candidate.available)
    {
      forcedAssignment = !candidate.available;
      callAssigned = assignCall(candidate.elevator);
      break;
    }
  }

  if (!callAssigned)
  {
    m_log.Trace("FORCED ASSIGNATION FOR CALL " + call->ToString(), Log::TraceLevel::Warning);
    forcedAssignment = true;
    callAssigned = assignCall(candidates.front().elevator);
  }

  Statistics::RecordAssignmentDecision(
    std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - assignmentStartTime),
    forcedAssignment,
    availableElevatorFound);

  return callAssigned;
}

void Management::TraceStatistics()
{
  const auto statistics = Statistics::GetSnapshot();
  std::vector<std::string> report;

  // Build the final report as a complete buffer and render it outside the rolling
  // log window, otherwise long tables can be truncated by the normal 15-line log.
  report.push_back("Simulation statistics");
  report.push_back(Separator(46));
  report.push_back("| Metric                 | Value             |");
  report.push_back(Separator(46));

  const auto addGlobalStat = [&report](const std::string& metric, const std::string& value)
  {
    std::stringstream row;
    row << "| " << std::left << std::setw(22) << metric
        << " | " << std::right << std::setw(17) << value
        << " |";
    report.push_back(row.str());
  };

  addGlobalStat("Queued calls", std::to_string(statistics.queuedCalls));
  addGlobalStat("Assigned calls", std::to_string(statistics.assignedCalls));
  addGlobalStat("Boarded passengers", std::to_string(statistics.boardedPassengers));
  addGlobalStat("Completed passengers", std::to_string(statistics.completedPassengers));
  addGlobalStat("Simulation time", Configuration::Simulation::CurrentDayTimeLabel());
  addGlobalStat("Average wait", FormatSeconds(statistics.AverageWaitTimeMs()));
  addGlobalStat("Max wait", FormatSeconds(static_cast<double>(statistics.maxWaitTimeMs)));

  report.push_back(Separator(46));
  report.push_back("");

  report.push_back("Algorithm anomaly indicators");
  report.push_back(Separator(84));
  report.push_back("| Indicator                     | Value             | Level | Notes              |");
  report.push_back(Separator(84));

  const auto addIndicator = [&report](
    const std::string& indicator,
    const std::string& value,
    const std::string& level,
    const std::string& notes)
  {
    std::stringstream row;
    row << "| " << std::left << std::setw(29) << indicator
        << " | " << std::right << std::setw(17) << value
        << " | " << std::left << std::setw(5) << level
        << " | " << std::setw(18) << notes
        << " |";
    report.push_back(row.str());
  };

  const auto pendingBoarding =
    statistics.queuedCalls > statistics.boardedPassengers ? statistics.queuedCalls - statistics.boardedPassengers : 0U;
  const auto pendingCompletion =
    statistics.boardedPassengers > statistics.completedPassengers ? statistics.boardedPassengers - statistics.completedPassengers : 0U;
  const auto maxWaitRatio =
    statistics.AverageWaitTimeMs() <= 0.0 ? 0.0 : static_cast<double>(statistics.maxWaitTimeMs) / statistics.AverageWaitTimeMs();
  const auto fleetSizing = BuildFleetSizingRecommendation(statistics, m_elevators, pendingBoarding);
  const auto cabinSizing = BuildCabinSizingRecommendation(statistics);

  // These indicators are intentionally heuristic. They are meant to point the
  // operator toward suspicious behavior, not to prove that the dispatch is wrong.
  addIndicator(
    "Forced assignments",
    std::to_string(statistics.forcedAssignments),
    statistics.forcedAssignments == 0U ? "OK" : "WARN",
    "no ideal car");

  addIndicator(
    "No available elevator",
    std::to_string(statistics.noAvailableElevatorDecisions),
    statistics.noAvailableElevatorDecisions == 0U ? "OK" : "WARN",
    "fallback used");

  addIndicator(
    "Capacity limit waits",
    std::to_string(statistics.capacityLimitedDecisions),
    statistics.capacityLimitedDecisions == 0U ? "OK" : "WARN",
    "full cabins");

  addIndicator(
    "Pending boarding",
    std::to_string(pendingBoarding),
    pendingBoarding == 0U ? "OK" : "WARN",
    "calls waiting");

  addIndicator(
    "Pending completion",
    std::to_string(pendingCompletion),
    pendingCompletion == 0U ? "OK" : "INFO",
    "people in transit");

  addIndicator(
    "Avg assignment time",
    FormatMilliseconds(statistics.AverageAssignmentTimeMs()),
    statistics.AverageAssignmentTimeMs() < 10.0 ? "OK" : "WARN",
    "dispatch timing");

  addIndicator(
    "Max assignment time",
    FormatMilliseconds(static_cast<double>(statistics.maxAssignmentTimeUs) / 1000.0),
    statistics.maxAssignmentTimeUs < 50000U ? "OK" : "WARN",
    "dispatch spike");

  addIndicator(
    "Max wait / avg wait",
    statistics.boardedPassengers == 0U ? "n/a" : FormatRatio(maxWaitRatio),
    maxWaitRatio <= 2.5 ? "OK" : "WARN",
    "tail latency");

  report.push_back(Separator(84));
  report.push_back("");

  report.push_back("Fleet sizing recommendation");
  report.push_back(Separator(80));
  report.push_back("| Parameter                     | Value             | Notes              |");
  report.push_back(Separator(80));

  const auto addSizingParameter = [&report](
    const std::string& parameter,
    const std::string& value,
    const std::string& notes)
  {
    std::stringstream row;
    row << "| " << std::left << std::setw(29) << parameter
        << " | " << std::right << std::setw(17) << value
        << " | " << std::left << std::setw(18) << notes
        << " |";
    report.push_back(row.str());
  };

  addSizingParameter("Configured elevators", std::to_string(Configuration::Building::NumberOfElevators()), "config input");
  addSizingParameter("Recommended elevators", std::to_string(fleetSizing.recommendedElevators), fleetSizing.signal);
  addSizingParameter("Sizing reason", fleetSizing.reason, "heuristic");
  addSizingParameter("Target avg wait", FormatSeconds(fleetSizing.targetAverageWaitMs), "service target");
  addSizingParameter("Observed avg wait", FormatSeconds(statistics.AverageWaitTimeMs()), "simulation");
  addSizingParameter("Forced assignment rate", FormatPercent(fleetSizing.forcedAssignmentRatio), "pressure");
  addSizingParameter("No-available rate", FormatPercent(fleetSizing.noAvailableRatio), "saturation");
  addSizingParameter("Unassigned elevators", std::to_string(fleetSizing.unassignedElevators), "possible spare");

  report.push_back(Separator(80));
  report.push_back("");

  report.push_back("Cabin sizing recommendation");
  report.push_back(Separator(80));
  report.push_back("| Parameter                     | Value             | Notes              |");
  report.push_back(Separator(80));

  addSizingParameter("Configured max people", std::to_string(Configuration::Elevator::MaxPeople()), "config input");
  addSizingParameter("Recommended max people", std::to_string(cabinSizing.recommendedMaxPeople), cabinSizing.signal);
  addSizingParameter("Sizing reason", cabinSizing.reason, "heuristic");
  addSizingParameter("Peak people on board", std::to_string(statistics.maxCabinPeople), "observed");
  addSizingParameter("Peak committed load", std::to_string(statistics.maxCommittedElevatorLoad), "assigned+boarded");
  addSizingParameter("Capacity limit waits", std::to_string(statistics.capacityLimitedDecisions), "pressure");

  report.push_back(Separator(80));
  report.push_back("");

  report.push_back("Elevator statistics");
  report.push_back(Separator(103));
  report.push_back("| Elevator | Assigned calls | Travelled floors | Runs | Door cycles | Queued stops | People on board |");
  report.push_back(Separator(103));

  for (const auto& elevator : m_elevators)
  {
    std::stringstream row;
    row << "| " << std::left << std::setw(8) << elevator->GetId()
        << " | " << std::right << std::setw(14) << AssignedCallsForElevator(statistics, elevator->GetId())
        << " | " << std::right << std::setw(16) << elevator->GetTotalFloorsTravelled()
        << " | " << std::setw(4) << elevator->GetRunCounter()
        << " | " << std::setw(11) << elevator->GetDoorCycles()
        << " | " << std::setw(12) << elevator->GetQueuedStopsCount()
        << " | " << std::setw(15) << elevator->GetPeopleCount()
        << " |";
    report.push_back(row.str());
  }

  report.push_back(Separator(103));

  LogBase::FlushPendingMessages();

  if (Configuration::Log::WriteToScreen())
    ConsoleView::Instance().WriteFinalReport(report);

  if (Configuration::Log::WriteToFile())
    WriteFinalReportToFile(report);
}


std::vector<ElevatorSnapshot> Management::GetElevatorSnapshots() const
{
  std::vector<ElevatorSnapshot> snapshots;
  snapshots.reserve(m_elevators.size());

  for (const auto& elevator : m_elevators)
    snapshots.push_back(elevator->GetSnapshot());

  std::sort(snapshots.begin(), snapshots.end(),
    [](const auto& a, const auto& b) { return a.id < b.id; });

  return snapshots;
}
