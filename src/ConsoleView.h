/**********************************************************************************
*        File: ConsoleView.h
* Description: Shared console layout for dashboard and logs.
**********************************************************************************/

#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <vector>

class ConsoleView final
{
public:
  static ConsoleView& Instance();

  ConsoleView(const ConsoleView&) = delete;
  ConsoleView(ConsoleView&&) = delete;

  ConsoleView& operator=(const ConsoleView&) = delete;
  ConsoleView& operator=(ConsoleView&&) = delete;

public:
  void SetDashboardHeight(unsigned int height);
  void DrawDashboard(const std::vector<std::string>& lines);
  void WriteLog(const std::string& line);
  void Shutdown();

private:
  ConsoleView() = default;
  ~ConsoleView();

  void Initialize();
  void DrawLogArea();
  unsigned int FirstLogRow() const;

private:
  std::mutex m_mutex;
  bool m_initialized{ false };
  unsigned int m_dashboardHeight{ 10 };
  unsigned int m_logHeight{ 15 };
  std::deque<std::string> m_logLines;
};
