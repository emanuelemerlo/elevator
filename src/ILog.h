/**********************************************************************************
*        File: ILog.h
* Description: Interface of the log service.
*      Author: Emanuele Merlo (emanuele.merlo@gmail.com)
*       Notes:
**********************************************************************************/

#pragma once

#include <string>

/**
 * \brief Basic interface for log services.
 */
struct ILog
{
  enum class LogType { Screen = 0, File, ScreenAndFile, Default = Screen };

  enum class TraceLevel { Debug, Verbose, Info, Warning, Error };

  /**
   * \brief Trace a message on the standard output.
   * \param message Message to trace.
   * \param level [Optional] Trace level used to filter trace messages.
   * \param messageSpecificId [Optional] Specific message id, a string printed before the message to identify the trace source. If empty the global trace trace id is used.
   */
  virtual void Trace(
    const std::stringstream& message, 
    const TraceLevel level = TraceLevel::Info, 
    const std::string& messageSpecificId = "") = 0;

  /**
   * \brief Trace a message on the standard output.
   * \param message Message to trace.
   * \param level [Optional] Trace level used to filter trace messages.
   * \param messageSpecificId [Optional] Specific message id, a string printed before the message to identify the trace source. If empty the general trace trace id is used.
   */
  virtual void Trace(
    const std::string& message, 
    const TraceLevel level = TraceLevel::Info, 
    const std::string& messageSpecificId = "") = 0;

  /**
   * \brief Trace a message on the standard output.
   * \param traceId Set the general trade Id: a string printed before the message to identify the trace source.
   */
  virtual void SetTraceId(const std::string& traceId) = 0;

  /**
   * \brief Trace a message on the standard output.
   * \return Get the general trace Id: a string printed before the message to identify the trace source.
   */
  virtual const std::string& GetTraceId() const = 0;
  
  /**
   * \brief Set the trace level filter threshold.
   * \param startLevel Filter threshold below which messages are not traced.
   */
  virtual void SetTraceLevelFilter(const TraceLevel startLevel) = 0;
};
