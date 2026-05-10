#include "Management.h"

#include "Elevator.h"

#include <random>
#include <cstdlib>
#include <sstream>
#include <algorithm>

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
  if (m_elevators.empty())
    return;

  m_log.Trace("Shutdown in progress...", Log::TraceLevel::Verbose);

  for (auto& elevator : m_elevators)
    elevator->ShutDown();

  m_elevators.clear();

  m_log.Trace("Shutdown completed", Log::TraceLevel::Verbose);
}

bool Management::AssignCall(std::shared_ptr<Call>& call)
{
  bool callAssigned = false;

  const auto assignCall = [&call, this](const auto& elevator)
  {
    std::stringstream message;
    message << "Call " << call->ToString() << " assigned to elevator: " << elevator->GetId();
    m_log.Trace(message);

    call->SetAssignedElevator(elevator->GetId());
    elevator->AnswerToCall(call);
  };

  const auto floorDifference = [&call](const auto& elevator) { return std::abs(static_cast<int>(elevator->GetCurrentFloor() - call->GetStartFloor())); };

  std::sort(m_elevators.begin(), m_elevators.end(),
    [&floorDifference](const auto& a, const auto& b) { return floorDifference(a) > floorDifference(b); });

  for(auto& elevator : m_elevators)
  {
    if ( m_elevators.size() == 1 || elevator->Available(call))
    {
      assignCall(elevator);
      callAssigned = true;
      break;
    }
  }

  if (!callAssigned)
  {
    m_log.Trace("FORCED ASSIGNATION FOR CALL " + call->ToString(), Log::TraceLevel::Warning);
   assignCall(*m_elevators.begin());
  }

  return callAssigned;
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
