#include "SymbolicInterface.h"

#include "Configuration.h"
#include "ConsoleView.h"
#include "Elevator.h"
#include "Floors.h"
#include "People.h"

#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace
{
  std::string DirectionSymbol(const Direction direction)
  {
    switch (direction)
    {
    case Direction::Up:
      return "^";
    case Direction::Down:
      return "v";
    case Direction::Both:
      return "|";
    case Direction::None:
    default:
      return "-";
    }
  }

  std::string StatusLabel(const ElevatorStatus status)
  {
    switch (status)
    {
    case ElevatorStatus::OutOfOrder:
      return "OUT";
    case ElevatorStatus::PeopleEnterAndExit:
      return "PAX";
    case ElevatorStatus::Moving:
      return "MOV";
    case ElevatorStatus::Idle:
    default:
      return "IDL";
    }
  }

  std::string WaitingLabel(const std::vector<std::pair<std::string, std::size_t>>& waitingPeople)
  {
    if (waitingPeople.empty())
      return "";

    std::stringstream label;

    for (auto assignment = waitingPeople.begin(); assignment != waitingPeople.end(); ++assignment)
    {
      if (assignment != waitingPeople.begin())
        label << " ";

      label << assignment->first << ":" << assignment->second;
    }

    return label.str();
  }

  std::size_t WaitingColumnWidth(const std::vector<ElevatorSnapshot>& snapshots)
  {
    std::size_t width = 0;

    for (const auto& elevator : snapshots)
      width += elevator.id.size() + 3U;

    if (!snapshots.empty())
      width += snapshots.size() - 1U;

    return width;
  }
}

SymbolicInterface::SymbolicInterface(Management& management) : m_management(management)
{
}

SymbolicInterface::~SymbolicInterface()
{
  Stop();
}

void SymbolicInterface::Start()
{
  if (m_thread != nullptr)
    return;

  ConsoleView::Instance().SetDashboardHeight(Configuration::Building::NumberOfFloors() + 5);
  m_stopRequested = false;
  m_thread = std::make_unique<std::thread>(&SymbolicInterface::ThreadFunction, this);
}

void SymbolicInterface::Stop()
{
  if (m_thread == nullptr)
    return;

  m_stopRequested = true;

  if (m_thread->joinable())
    m_thread->join();

  m_thread.reset();
}

void SymbolicInterface::ThreadFunction()
{
  while (!m_stopRequested)
  {
    Render();
    std::this_thread::sleep_for(500ms);
  }
}

void SymbolicInterface::Render() const
{
  const auto snapshots = m_management.GetElevatorSnapshots();

  std::vector<std::string> screen;
  std::stringstream line;
  const auto waitingColumnWidth = WaitingColumnWidth(snapshots);

  screen.emplace_back("=== Elevator symbolic interface ===");
  screen.emplace_back("Legend: [ID Dir People]  Dir: ^ up, v down, - stopped  A:2 = two waiting people assigned to elevator A");
  screen.emplace_back("");

  for (auto floor = static_cast<int>(Floors::TopFloor()); floor >= static_cast<int>(Floors::BottomFloor); --floor)
  {
    const auto waitingPeople = WaitingLabel(
      Floors::GetPeople().CountByStartFloorAndElevator(static_cast<Floors::FloorNumber>(floor)));

    line.str("");
    line.clear();
    line << "F" << std::setw(2) << floor << " |";

    if (!waitingPeople.empty())
      line << " " << std::left << std::setw(static_cast<int>(waitingColumnWidth)) << waitingPeople << std::right << " |";
    else
      line << " " << std::setw(static_cast<int>(waitingColumnWidth)) << "" << " |";

    for (const auto& elevator : snapshots)
    {
      if (static_cast<int>(elevator.currentFloor) == floor)
      {
        line << " [" << elevator.id << DirectionSymbol(elevator.direction)
             << std::setw(2) << std::setfill('0') << elevator.peopleCount << std::setfill(' ') << "]";
      }
      else
      {
        line << " [    ]";
      }
    }

    screen.emplace_back(line.str());
  }

  screen.emplace_back("");
  line.str("");
  line.clear();
  line << "Status:";
  for (const auto& elevator : snapshots)
  {
    line << "  " << elevator.id << "=" << StatusLabel(elevator.status)
         << " F" << elevator.currentFloor
         << " " << DirectionSymbol(elevator.direction)
         << " people=" << elevator.peopleCount;
  }
  screen.emplace_back(line.str());

  ConsoleView::Instance().DrawDashboard(screen);
}
