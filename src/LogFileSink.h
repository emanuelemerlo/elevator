/**********************************************************************************
*        File: LogFileSink.h
* Description: Shared file sink for log output.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace LogFileSink
{
  void WriteLine(const std::string& line);
  void WriteLines(const std::vector<std::string>& lines);
}
