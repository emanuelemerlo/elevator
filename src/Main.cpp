#include "Management.h"
#include "PeopleCallsGenerator.h"
#include "Configuration.h"
#include "Log.h"
#include "Statistics.h"
#include "SymbolicInterface.h"

#include <iostream>
#include <string> 

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
    symbolicInterface.Start();

    switch (Configuration::CallsGenerator::GeneratorType())
    {
    case Configuration::CallsGenerator::Type::Random:
      callsGenerator.StartRandom(/*Configuration::CallsGenerator::NumberOfCalls()*/);
      break;
    case Configuration::CallsGenerator::Type::Fixed:
    default:
      callsGenerator.StartFixed();
    }

    std::cin.get();
    log.Trace("Shutdown requested...");

    symbolicInterface.Stop();
    callsGenerator.Shutdown();
    elevatorsManagement.Shutdown();
    elevatorsManagement.TraceStatistics();
  }
  catch(std::exception& e)
  {
    log.Trace(std::string("** CAUGHT EXCEPTION ** ") + e.what(), Log::TraceLevel::Error);
  }

  std::this_thread::sleep_for(std::chrono::seconds(30));
  return 0;
}
