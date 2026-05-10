#include "ConsoleView.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
  constexpr const char* Ansi = "\x1b[";
  constexpr unsigned int ScreenPadding = 1;

  unsigned int ConsoleWidth()
  {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo{};
    const HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (outputHandle != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(outputHandle, &screenBufferInfo))
      return static_cast<unsigned int>(screenBufferInfo.dwSize.X);
#endif

    return 120;
  }

  std::string ClipToConsoleWidth(const std::string& line)
  {
    const unsigned int width = ConsoleWidth();
    if (width <= 1 || line.size() < width)
      return line;

    return line.substr(0, width - 1);
  }
}

ConsoleView& ConsoleView::Instance()
{
  static ConsoleView consoleView;
  return consoleView;
}

ConsoleView::~ConsoleView()
{
  Shutdown();
}

void ConsoleView::SetDashboardHeight(const unsigned int height)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_dashboardHeight = height;
}

void ConsoleView::DrawDashboard(const std::vector<std::string>& lines)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  Initialize();

  for (unsigned int row = 0; row < m_dashboardHeight; ++row)
  {
    std::cout << Ansi << (row + 1) << ";1H" << Ansi << "2K";

    if (row < lines.size())
      std::cout << lines[row];
  }

  std::cout << Ansi << (FirstLogRow() + m_logHeight) << ";1H" << std::flush;
}

void ConsoleView::WriteLog(const std::string& line)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  Initialize();

  m_logLines.push_back(line);
  while (m_logLines.size() > m_logHeight)
    m_logLines.pop_front();

  DrawLogArea();
}

void ConsoleView::Shutdown()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_initialized)
    return;

  std::cout << Ansi << (FirstLogRow() + m_logHeight) << ";1H"
            << Ansi << "?25h" << std::flush;
  m_initialized = false;
}

void ConsoleView::Initialize()
{
  if (m_initialized)
    return;

#ifdef _WIN32
  const HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (outputHandle != INVALID_HANDLE_VALUE)
  {
    DWORD consoleMode = 0;
    if (GetConsoleMode(outputHandle, &consoleMode))
      SetConsoleMode(outputHandle, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
#endif

  std::cout << Ansi << "2J" << Ansi << "?25l";
  m_initialized = true;
  DrawLogArea();
}

void ConsoleView::DrawLogArea()
{
  const unsigned int firstRow = FirstLogRow();

  for (unsigned int row = 0; row < m_logHeight; ++row)
  {
    std::cout << Ansi << (firstRow + row) << ";1H" << Ansi << "2K";

    if (row < m_logLines.size())
      std::cout << ClipToConsoleWidth(m_logLines[row]);
  }

  std::cout << Ansi << (firstRow + m_logHeight) << ";1H" << std::flush;
}

unsigned int ConsoleView::FirstLogRow() const
{
  return m_dashboardHeight + ScreenPadding + 1;
}
