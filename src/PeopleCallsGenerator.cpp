#include "PeopleCallsGenerator.h"

#include "Call.h"
#include "Management.h"
#include "Floors.h"
#include "People.h"
#include "Watchdog.h"
#include "Configuration.h"
#include "Statistics.h"

#include <random>
#include <chrono>
#include <future>
#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>

using namespace std::chrono_literals;

PeopleCallsGenerator::PeopleCallsGenerator(Management& management) : m_management(management)
{
  m_log.SetTraceId("Generator");
}

PeopleCallsGenerator::~PeopleCallsGenerator()
{
  Shutdown();
}

namespace
{
  double DailyTrafficWeight(const double hour)
  {
    const auto peak = [hour](const double center, const double width, const double weight)
    {
      const auto distance = hour - center;
      return weight * std::exp(-(distance * distance) / (2.0 * width * width));
    };

    const auto morning = peak(8.25, 1.25, 1.00);
    const auto lunch = peak(12.50, 1.00, 0.70);
    const auto dinner = peak(19.00, 1.35, 0.75);
    return 0.25 + morning + lunch + dinner;
  }

  double AverageDailyTrafficWeight()
  {
    constexpr unsigned int SamplesPerDay = 96U;

    double totalWeight = 0.0;
    for (auto sample = 0U; sample < SamplesPerDay; ++sample)
    {
      const auto hour = static_cast<double>(sample) / static_cast<double>(SamplesPerDay) * 24.0;
      totalWeight += DailyTrafficWeight(hour);
    }

    return totalWeight / static_cast<double>(SamplesPerDay);
  }

  double DailyTrafficIntensity()
  {
    return std::max(0.1, DailyTrafficWeight(Configuration::Simulation::CurrentHour()) / AverageDailyTrafficWeight());
  }

  std::chrono::milliseconds DelayForDailyProfile(const double jitter)
  {
    const auto averageCallsPerDay = static_cast<double>(Configuration::CallsGenerator::AverageCallsPerDay());
    const auto simulatedDayDurationMs = static_cast<double>(Configuration::CallsGenerator::SimulationDayDuration());
    const auto averageDelayMs = simulatedDayDurationMs / averageCallsPerDay;
    const auto profiledDelayMs = averageDelayMs / DailyTrafficIntensity() * jitter;

    const auto minDelayMs = Configuration::CallsGenerator::MinDelayBetweenCalls();
    const auto maxDelayMs = Configuration::CallsGenerator::MaxDelayBetweenCalls();

    const long long lowerBound = std::max(1LL, minDelayMs);
    const long long upperBound = std::max(lowerBound, maxDelayMs);

    long long boundedDelayMs = static_cast<long long>(profiledDelayMs);
    if (boundedDelayMs < lowerBound)
      boundedDelayMs = lowerBound;
    if (boundedDelayMs > upperBound)
      boundedDelayMs = upperBound;

    return std::chrono::milliseconds(boundedDelayMs);
  }

  bool MaxConcurrentCallsReached()
  {
    return Statistics::ActiveCalls() >= Configuration::CallsGenerator::MaxConcurrentCalls();
  }

  // Delays are configured in simulated time: the time scale decides how long they last for real.
  std::chrono::microseconds Real(const std::chrono::milliseconds simulatedDuration)
  {
    return Configuration::Simulation::ToRealDuration(simulatedDuration);
  }

  unsigned int GeneratorSeed()
  {
    const auto configuredSeed = Configuration::Simulation::RandomSeed();
    if (configuredSeed != 0U)
      return configuredSeed;

    return static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
  }
}

void PeopleCallsGenerator::StartRandom()
{
  m_finished = false;

  if (m_generatorThread != nullptr && m_generatorThread->IsActive())
    m_generatorThread->Stop();

  m_generatorThread = std::make_unique<RandomGeneratorThread>(this);
  m_generatorThread->Start();
  m_generatorThread->Go();
}

void PeopleCallsGenerator::StartFixed()
{
  m_finished = false;

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

  if (!SleepFor(Real(2s))) // arbitrary delay before start, interrupted by a stop request
    return;

  // Same seed -> same sequence of calls (floors and delays); the seed is traced to reproduce a run.
  const auto seed = GeneratorSeed();
  peopleCallsGenerator->m_log.Trace("Random seed: " + std::to_string(seed), Log::TraceLevel::Info);
  std::default_random_engine generator(seed);

  const std::uniform_int_distribution<Floors::FloorNumber> randomFloor(Floors::BottomFloor, Floors::TopFloor());
  const std::uniform_real_distribution<double> randomJitter(0.75, 1.25);

  const auto maxCalls = Configuration::CallsGenerator::NumberOfCalls();
  unsigned int generatedCalls = 0U;

  do
  {
    if (Configuration::Simulation::Completed())
    {
      peopleCallsGenerator->m_log.Trace("Simulation days completed: generator stopped");
      break;
    }

    if (generatedCalls >= maxCalls)
    {
      peopleCallsGenerator->m_log.Trace("Configured number of calls reached: generator stopped");
      break;
    }

    if (MaxConcurrentCallsReached())
    {
      if (!SleepFor(Real(DelayForDailyProfile(1.0))))
        break;

      continue;
    }

    std::shared_ptr<Call> call;

    do
    {
      auto getStartFloor = std::bind(randomFloor, std::ref(generator));
      auto getDestinationFloor = std::bind(randomFloor, std::ref(generator));

      call = std::make_shared<Call>(getStartFloor(), getDestinationFloor());
    } while (!call->IsValid()); // only valid calls

    peopleCallsGenerator->m_log.Trace("Generated call " + call->ToString());

    Floors::GetPeople().Insert(call);

    // Assign call synchronously to avoid unbounded thread creation
    peopleCallsGenerator->m_management.AssignCall(call);
    ++generatedCalls;

    auto getJitter = std::bind(randomJitter, std::ref(generator));

    // Interruptible sleep: a shutdown request must not wait for the next call delay
    if (!SleepFor(Real(DelayForDailyProfile(getJitter()))))
      break;

  } while (!StopRequested());

  peopleCallsGenerator->m_finished = true;
  peopleCallsGenerator->m_log.Trace("CycleFunction exit", ILog::TraceLevel::Debug);
}

void PeopleCallsGenerator::FixedGeneratorThread::CycleFunction(PeopleCallsGenerator* peopleCallsGenerator)
{
  if (peopleCallsGenerator == nullptr)
    return;

  std::default_random_engine generator(GeneratorSeed());

  const std::uniform_int_distribution<long long> randomDelay(
    Configuration::CallsGenerator::MinDelayBetweenCalls(),
    Configuration::CallsGenerator::MaxDelayBetweenCalls());

  if (!SleepFor(Real(2s))) // arbitrary delay before start, interrupted by a stop request
    return;

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
    if (StopRequested())
      break;

    if (!call->IsValid())
      continue;

    peopleCallsGenerator->m_log.Trace("Asking call assignment " + call->ToString());

    Floors::GetPeople().Insert(call);

    // Assign call synchronously
    peopleCallsGenerator->m_management.AssignCall(call);

    auto getDelay = std::bind(randomDelay, std::ref(generator));

    // Interruptible sleep: a shutdown request must not wait for the remaining calls
    if (!SleepFor(Real(std::chrono::milliseconds(getDelay()))))
      break;
  }

  peopleCallsGenerator->m_finished = true;
}

