#include "Management.h"
#include "PeopleCallsGenerator.h"
#include "Configuration.h"
#include "Log.h"
#include "LogBase.h"
#include "Statistics.h"
#include "SymbolicInterface.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string> 
#include <thread>

namespace
{
  // State shared with the thread that waits for the Enter key. It is heap allocated and shared because that
  // thread stays blocked on stdin and can outlive main() when the simulation ends by itself.
  struct StopSignal
  {
    std::mutex mutex;
    std::condition_variable condition;
    bool stopRequested{ false };
  };

  void WaitForEnterKey(const std::shared_ptr<StopSignal> signal)
  {
    const auto key = std::cin.get();

    // End of input (non interactive run, e.g. stdin redirected): with the automatic stop enabled the
    // simulation just keeps going until it completes; without it nothing else could stop it.
    if (key == std::char_traits<char>::eof() && Configuration::Simulation::AutoStop())
      return;

    {
      std::lock_guard<std::mutex> lock(signal->mutex);
      signal->stopRequested = true;
    }

    signal->condition.notify_all();
  }

  // Waits until the user presses Enter or, with the automatic stop enabled, until every call has been generated
  // and served. Returns true if the simulation completed by itself.
  bool WaitForEndOfSimulation(const std::shared_ptr<StopSignal>& signal, const PeopleCallsGenerator& callsGenerator)
  {
    // If the generator is done but passengers stop making progress, the run is closed anyway:
    // the final report shows what is left (pending boarding / completion).
    constexpr long long NoProgressLimitMs = 5LL * 60LL * 1000LL;

    auto lastProgressCount = std::uint64_t{ 0U };
    auto lastProgressTimeMs = Configuration::Simulation::ElapsedMs();

    std::unique_lock<std::mutex> lock(signal->mutex);

    while (!signal->stopRequested)
    {
      signal->condition.wait_for(lock, std::chrono::milliseconds(50), [&signal]() { return signal->stopRequested; });

      if (!Configuration::Simulation::AutoStop() || !callsGenerator.Finished())
        continue;

      if (Statistics::ActiveCalls() == 0U)
        return true;

      const auto snapshot = Statistics::GetSnapshot();
      const auto progressCount = snapshot.boardedPassengers + snapshot.completedPassengers;
      const auto nowMs = Configuration::Simulation::ElapsedMs();

      if (progressCount != lastProgressCount)
      {
        lastProgressCount = progressCount;
        lastProgressTimeMs = nowMs;
      }
      else if (nowMs - lastProgressTimeMs > NoProgressLimitMs)
      {
        return true;
      }
    }

    return false;
  }
}

int main()
{
  try
  {
    Configuration::LoadFromFile("config.json");
  }
  catch(std::exception& e)
  {
    std::cerr << "Invalid configuration: " << e.what() << std::endl;
    return 1;
  }

  Log log;
  log.Trace("Press Enter to stop...");

  try
  {
    Statistics::Reset();
    Configuration::Simulation::StartClock();

    Management elevatorsManagement(Configuration::Building::NumberOfElevators());

    PeopleCallsGenerator callsGenerator(elevatorsManagement);
    SymbolicInterface symbolicInterface(elevatorsManagement);
    // The dashboard is drawn on the console: do not draw it when logging only to file.
    if (Configuration::Log::WriteToScreen())
      symbolicInterface.Start();

    switch (Configuration::CallsGenerator::GeneratorType())
    {
    case Configuration::CallsGenerator::Type::Random:
      callsGenerator.StartRandom();
      break;
    case Configuration::CallsGenerator::Type::Fixed:
    default:
      callsGenerator.StartFixed();
    }

    const auto stopSignal = std::make_shared<StopSignal>();
    std::thread(WaitForEnterKey, stopSignal).detach();

    const auto completed = WaitForEndOfSimulation(stopSignal, callsGenerator);
    log.Trace(completed ? "Simulation completed: all calls served, shutting down..." : "Shutdown requested...");

    symbolicInterface.Stop();
    callsGenerator.Shutdown();
    elevatorsManagement.Shutdown();
    elevatorsManagement.TraceStatistics();
  }
  catch(std::exception& e)
  {
    log.Trace(std::string("** CAUGHT EXCEPTION ** ") + e.what(), Log::TraceLevel::Error);
  }

  // Make sure every pending message (final report, error messages) has been written before exiting.
  LogBase::FlushPendingMessages();
  return 0;
}
