/**********************************************************************************
*        File: SymbolicInterface.h
* Description: Console symbolic interface for elevator status.
**********************************************************************************/

#pragma once

#include "Management.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <memory>

using namespace std::chrono_literals;

/**
 * \brief Periodically draws a compact, symbolic view of all elevators.
 */
class SymbolicInterface final
{
public:
  explicit SymbolicInterface(Management& management);
  ~SymbolicInterface();

  SymbolicInterface(const SymbolicInterface&) = delete;
  SymbolicInterface(SymbolicInterface&&) = delete;

  SymbolicInterface& operator=(const SymbolicInterface&) = delete;
  SymbolicInterface& operator=(SymbolicInterface&&) = delete;

public:
  void Start();
  void Stop();

private:
  void ThreadFunction();
  void Render() const;

private:
  Management& m_management;
  std::unique_ptr<std::thread> m_thread;
  std::atomic_bool m_stopRequested{ false };
};
