#include "People.h"
#include "Log.h"
#include "Statistics.h"

#include <map>
#include <sstream>

void People::EnterAndExit(
  People& waitingPeople, 
  const Floors::FloorNumber currentFloor, 
  const Direction currentDirection, 
  const std::string& elevatorId)
{
  waitingPeople.Trace(currentFloor);
  Enter(waitingPeople, currentFloor, currentDirection, elevatorId);
  Exit(currentFloor);
}

void People::Insert(const std::shared_ptr<Call>& call)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  call->MarkQueued();
  Statistics::RecordQueuedCall();

  push_front(call);
}

void People::Trace(const Floors::FloorNumber currentFloor)
{
  std::stringstream message;

  if (currentFloor != Floors::InvalidFloor)
  {
    message << "People waiting on floor " << std::to_string(currentFloor) << ": ";

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& person : *this)
      if ((*person).GetStartFloor() == currentFloor)
        message << (*person).ToString();
  }
  else
  {
    message << "People waiting on floors: ";

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& person : *this)
        message << (*person).ToString();
  }

  m_log.Trace(message);
}

bool People::Empty() 
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return empty();
}

std::size_t People::Count() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return size();
}

std::size_t People::CountByStartFloor(const Floors::FloorNumber floor) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::size_t count = 0;
  for (const auto& person : *this)
    if (person->GetStartFloor() == floor)
      ++count;

  return count;
}

std::vector<std::pair<std::string, std::size_t>> People::CountByStartFloorAndElevator(const Floors::FloorNumber floor) const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  std::map<std::string, std::size_t> counts;
  for (const auto& person : *this)
    if (person->GetStartFloor() == floor)
      ++counts[person->GetAssignedElevator()];

  return std::vector<std::pair<std::string, std::size_t>>(counts.begin(), counts.end());
}

std::vector<std::shared_ptr<Call>> People::Snapshot() const
{
  // Return a copy of the shared_ptr list so callers can iterate without holding
  // the People mutex while elevator threads continue to board or exit passengers.
  std::lock_guard<std::mutex> lock(m_mutex);
  return std::vector<std::shared_ptr<Call>>(begin(), end());
}

void People::Enter(
  People& waitingPeople, 
  const Floors::FloorNumber currentFloor, 
  const Direction currentDirection, 
  const std::string& elevatorId)
{
  std::stringstream message;
  message << "People enter from floor " << std::to_string(currentFloor) << ": ";

  // Move matching calls atomically from the global waiting list into this cabin.
  // This prevents another elevator from boarding the same passenger concurrently.
  std::lock_guard<std::mutex> waitingPeopleLock(waitingPeople.m_mutex);
  auto person = waitingPeople.begin();

  while (person != waitingPeople.end())
  {
    if ((*person)->GetStartFloor() == currentFloor && (*person)->GetDirection() == currentDirection && (*person)->GetAssignedElevator() == elevatorId)
    {
      message << (*person)->ToString();

      std::lock_guard<std::mutex> lock(m_mutex);
      Statistics::RecordBoardedPassenger((*person)->GetWaitingTime());
      push_front(*person);

      person = waitingPeople.erase(person);
      continue;
    }
      
    ++person;
  }

  m_log.Trace(message);
}

void People::Exit(const Floors::FloorNumber currentFloor)
{
  std::stringstream message;
  message << "People exit to floor " << std::to_string(currentFloor) << ": ";

  std::lock_guard<std::mutex> lock(m_mutex);
  auto person = begin();

  while(person != end())
  {
    if ((*person)->GetDestinationFloor() == currentFloor)
    {
      message << (*person)->ToString();
      Statistics::RecordCompletedPassenger();
      person = erase(person);
      continue;
    }

    ++person;
  }

  m_log.Trace(message);
}
