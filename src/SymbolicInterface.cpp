#include "SymbolicInterface.h"

#include "Configuration.h"
#include "Elevator.h"
#include "Floors.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace
{
  std::string DirectionSymbol(const Direction direction)
  {
    switch (direction)
    {
    case Direction::Up:
      return "↑";
    case Direction::Down:
      return "↓";
    case Direction::Both:
      return "↕";
    case Direction::None:
    default:
      return "·";
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

  std::stringstream screen;
  screen << "\n=== Elevator symbolic interface ===\n";
  screen << "Legend: [ID Direction People]  Direction: ↑ up, ↓ down, · stopped\n\n";

  for (auto floor = static_cast<int>(Floors::TopFloor); floor >= static_cast<int>(Floors::BottomFloor); --floor)
  {
    screen << "F" << std::setw(2) << floor << " |";

    for (const auto& elevator : snapshots)
    {
      if (static_cast<int>(elevator.currentFloor) == floor)
      {
        screen << " [" << elevator.id << DirectionSymbol(elevator.direction)
               << std::setw(2) << std::setfill('0') << elevator.peopleCount << std::setfill(' ') << "]";
      }
      else
      {
        screen << " [    ]";
      }
    }

    screen << '\n';
  }

  screen << "\nStatus:";
  for (const auto& elevator : snapshots)
  {
    screen << "  " << elevator.id << "=" << StatusLabel(elevator.status)
           << " F" << elevator.currentFloor
           << " " << DirectionSymbol(elevator.direction)
           << " people=" << elevator.peopleCount;
  }
  screen << "\n";

  std::cout << screen.str() << std::flush;
}
