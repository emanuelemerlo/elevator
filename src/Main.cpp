#include "Management.h"
#include "PeopleCallsGenerator.h"
#include "Configuration.h"
#include "Log.h"

#include <iostream>
#include <string> 

using namespace Configuration::CallsGenerator;

int main()
{
  const Log log;
  log.Trace("Press Enter to stop...");

  try
  {
    Management elevatorsManagement(Configuration::Building::NumberOfElevators);

    PeopleCallsGenerator callsGenerator(elevatorsManagement);

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

    callsGenerator.Shutdown();
    elevatorsManagement.Shutdown();
  }
  catch(std::exception& e)
  {
    log.Trace(std::string("** CAUGHT EXCEPTION ** ") + e.what(), Log::TraceLevel::Error);
  }

  return 0;
}