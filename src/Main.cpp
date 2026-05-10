#include "Management.h"
#include "PeopleCallsGenerator.h"
#include "Configuration.h"
#include "Log.h"
#include "SymbolicInterface.h"

#include <iostream>
#include <string> 

using namespace Configuration::CallsGenerator;

int main()
{
  Log log;
  log.Trace("Press Enter to stop...");

  try
  {
    Management elevatorsManagement(Configuration::Building::NumberOfElevators);

    PeopleCallsGenerator callsGenerator(elevatorsManagement);
    SymbolicInterface symbolicInterface(elevatorsManagement);
    symbolicInterface.Start();

    switch (GeneratorType)
    {
    case Type::Random:
      callsGenerator.StartRandom(NumberOfCalls);
      break;
    case Type::Fixed: 
    default:
      callsGenerator.StartFixed();
    }

    std::cin.get();
    log.Trace("Shutdown requested...");

    symbolicInterface.Stop();
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
