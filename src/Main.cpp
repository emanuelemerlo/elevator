#include "Management.h"
#include "PeopleCallsGenerator.h"
#include "Configuration.h"
#include "Log.h"

#include <iostream>
#include <string> 

int main()
{
  Configuration::LoadFromJsonFile();
  const auto& configuration = Configuration::Get();

  Log log;
  log.Trace("Press Enter to stop...");

  try
  {
    Management elevatorsManagement(configuration.building.numberOfElevators);

    PeopleCallsGenerator callsGenerator(elevatorsManagement);

    switch (configuration.callsGenerator.generatorType)
    {
    case Configuration::CallsGenerator::Type::Random:
      callsGenerator.StartRandom(configuration.callsGenerator.numberOfCalls);
      break;
    case Configuration::CallsGenerator::Type::Fixed:
    default:
      callsGenerator.StartFixed();
    }

    std::cin.get();
    log.Trace("Shutdown requested...");

    callsGenerator.Shutdown();
    elevatorsManagement.Shutdown();
  }
  catch(std::exception& e)
  {
    log.Trace(std::string("** CAUGHT EXCEPTION ** ") + e.what(), Log::TraceLevel::Error);
  }

  std::this_thread::sleep_for(std::chrono::seconds(10));

  return 0;
}
