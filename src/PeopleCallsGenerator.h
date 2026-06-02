/**********************************************************************************
*        File: PeopleCallsGenerator.h
* Description: Implements a random and a fixes calls generator.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include "Log.h"
#include "WorkerThread.h"

class PeopleCallsGenerator final 
{
private:
  class RandomGeneratorThread final : public WorkerThread<PeopleCallsGenerator>
  {
  public:
    explicit RandomGeneratorThread(PeopleCallsGenerator* peopleCallsGenerator) :
      WorkerThread<PeopleCallsGenerator>(peopleCallsGenerator) {}

  protected:
    void CycleFunction(PeopleCallsGenerator* peopleCallsGenerator) override;
  };

  class FixedGeneratorThread final : public WorkerThread<PeopleCallsGenerator>
  {
  public:
    explicit FixedGeneratorThread(PeopleCallsGenerator* peopleCallsGenerator = nullptr) :
      WorkerThread<PeopleCallsGenerator>(peopleCallsGenerator) {}

  protected:
    void CycleFunction(PeopleCallsGenerator* peopleCallsGenerator) override;
  };

public:
  explicit PeopleCallsGenerator(class Management& management);
  PeopleCallsGenerator() = delete;

  ~PeopleCallsGenerator();

  PeopleCallsGenerator(const PeopleCallsGenerator&) = delete;
  PeopleCallsGenerator& operator=(const PeopleCallsGenerator&) = delete;

  PeopleCallsGenerator(PeopleCallsGenerator&&) = delete;
  PeopleCallsGenerator operator=(PeopleCallsGenerator&&) = delete;

public:
  void StartRandom();
  void StartFixed();

  void Shutdown();

private:
  class Management& m_management;

  Log m_log;

  std::unique_ptr<WorkerThread<PeopleCallsGenerator>> m_generatorThread;
};


