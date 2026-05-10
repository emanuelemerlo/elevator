#include "PeopleCallsGenerator.h"

#include "Call.h"
#include "Management.h"
#include "Floors.h"
#include "People.h"
#include "Watchdog.h"
#include "Configuration.h"

#include <random>
#include <chrono>
#include <future>
#include <memory>
#include <functional>

using namespace std::chrono_literals;

PeopleCallsGenerator::PeopleCallsGenerator(Management& management) : m_management(management)
{
  m_log.SetTraceId("Generator");
}

PeopleCallsGenerator::~PeopleCallsGenerator()
{
  Shutdown();
}

void PeopleCallsGenerator::StartRandom(const unsigned int numberOfCalls)
{
  if (m_generatorThread != nullptr && m_generatorThread->IsActive())
    m_generatorThread->Stop();

  m_generatorThread = std::make_unique<RandomGeneratorThread>(this, numberOfCalls);
  m_generatorThread->Start();
  m_generatorThread->Go();
}

void PeopleCallsGenerator::StartFixed()
{
  if (m_generatorThread != nullptr && m_generatorThread->IsActive())
    m_generatorThread->Stop();

  m_generatorThread = std::make_unique<FixedGeneratorThread>(this);
  m_generatorThread->Start();
  m_generatorThread->Go();
}

void PeopleCallsGenerator::Shutdown()
{
  if (m_generatorThread == nullptr)
    return;

  m_generatorThread->Stop();
  m_generatorThread.reset();

  m_log.Trace("Shutdown completed", Log::TraceLevel::Verbose);
}

void PeopleCallsGenerator::RandomGeneratorThread::CycleFunction(PeopleCallsGenerator* peopleCallsGenerator)
{
  if (peopleCallsGenerator == nullptr || StopRequested())
    return;

  std::this_thread::sleep_for(2s); // arbitrary delay before start

  unsigned int numberOfGeneratedCalls = 0;

  const auto seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
  std::default_random_engine generator(seed);

  const std::uniform_int_distribution<Floors::FloorNumber> randomFloor(Floors::BottomFloor, Floors::TopFloor());
  const std::uniform_int_distribution<long long> randomDelay(
    Configuration::CallsGenerator::MinDelayBetweenCalls(),
    Configuration::CallsGenerator::MaxDelayBetweenCalls());

  do
  {
    std::shared_ptr<Call> call;

    do
    {
      auto getStartFloor = std::bind(randomFloor, std::ref(generator));
      auto getDestinationFloor = std::bind(randomFloor, std::ref(generator));

      call = std::make_shared<Call>(getStartFloor(), getDestinationFloor());
    } while (!call->IsValid()); // only valid calls

    peopleCallsGenerator->m_log.Trace("Generated call " + call->ToString());

    const auto it = Floors::GetPeople().Insert(call);

    // Async assign request
    auto handle = std::async(std::launch::async, [peopleCallsGenerator, &it]() {peopleCallsGenerator->m_management.AssignCall(*it); });

    // Synch assign request
    //m_management.AssignCall(*it);

    ++numberOfGeneratedCalls;
    if (m_numberOfCalls != Configuration::CallsGenerator::EndlessCalls && numberOfGeneratedCalls >= m_numberOfCalls)
      break;

    auto getDelay = std::bind(randomDelay, std::ref(generator));
    std::this_thread::sleep_for(std::chrono::milliseconds(getDelay()));

  } while (!StopRequested());

  peopleCallsGenerator->m_log.Trace("CycleFunction exit", ILog::TraceLevel::Debug);
}

void PeopleCallsGenerator::FixedGeneratorThread::CycleFunction(PeopleCallsGenerator* peopleCallsGenerator)
{
  if (peopleCallsGenerator == nullptr)
    return;

  const auto seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
  std::default_random_engine generator(seed);

  const std::uniform_int_distribution<long long> randomDelay(
    Configuration::CallsGenerator::MinDelayBetweenCalls(),
    Configuration::CallsGenerator::MaxDelayBetweenCalls());

  std::this_thread::sleep_for(2s); // arbitrary delay before start

  const auto topFloor = Floors::TopFloor();
  const auto bottomFloor = Floors::BottomFloor;

  std::list<std::shared_ptr<Call>> calls = {
    std::make_shared<Call>(2,1),
    std::make_shared<Call>(5,1),
    std::make_shared<Call>(6,1),
    std::make_shared<Call>(7,1),
    std::make_shared<Call>(8,1),

    std::make_shared<Call>(0,8),
    std::make_shared<Call>(2,8),
    std::make_shared<Call>(3,8),
    std::make_shared<Call>(4,8),
    std::make_shared<Call>(6,8),

    std::make_shared<Call>(1, topFloor),
    std::make_shared<Call>(3, topFloor),
    std::make_shared<Call>(5, topFloor),
    std::make_shared<Call>(7, topFloor),
    std::make_shared<Call>(8, topFloor),

    std::make_shared<Call>(bottomFloor, 2),
    std::make_shared<Call>(bottomFloor, 4),
    std::make_shared<Call>(bottomFloor, 6),
    std::make_shared<Call>(bottomFloor, 8),
    std::make_shared<Call>(bottomFloor, 9)
  };

  for (const auto& call : calls)
  {
    if (!call->IsValid())
      continue;

    peopleCallsGenerator->m_log.Trace("Asking call assignment " + call->ToString());

    const auto it = Floors::GetPeople().Insert(call);

    // Async assign request
    auto handle = std::async(std::launch::async, [peopleCallsGenerator, &it]() {peopleCallsGenerator->m_management.AssignCall(*it); });

    // Synch assign request
    //m_management.AssignCall(*it);

    auto getDelay = std::bind(randomDelay, std::ref(generator));
    std::this_thread::sleep_for(std::chrono::milliseconds(getDelay()));
  }
}

