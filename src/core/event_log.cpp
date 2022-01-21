#define _CRT_SECURE_NO_WARNINGS

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <mutex>
#include <string>
#include <iostream>
#include <fstream>
#include "event_log.hpp"
#include "../CommandVars.hpp"
#include "../ui/print.hpp"
#include "../util/util.hpp"

std::mutex logMutex;

void prepare_event_log_file() {
  if (g_comVars.eventLogFilePathname.size() == 0) {
    // no event log file was specified, do nothing
    return;
  }

  std::ofstream eventLogFile(g_comVars.eventLogFilePathname);
  assert_file(eventLogFile, g_comVars.eventLogFilePathname.c_str(), false);

  if (eventLogFile.is_open()) {
    eventLogFile << "Ant Simulator Lite Event Logs:\n";
    eventLogFile << "(INF=info, SUC=success, WRN=warning, ERR=error)\n";
    eventLogFile.close();
  }
}

const char * get_event_type_tag(EventType const type) {
  switch (type) {
    default:
    case EventType::Info:
      return "INF";
    case EventType::Success:
      return "SUC";
    case EventType::Warning:
      return "WRN";
    case EventType::Error:
      return "ERR";
  }
}

void construct_time_stamp(char * out, size_t outSize) {
  const time_t now = time(NULL);
  struct tm const * local = localtime(&now);

  snprintf(
    out,
    outSize,
    " - %02d:%02d:%02d",
    local->tm_hour,
    local->tm_min,
    local->tm_sec
  );
}

std::string construct_event_str(
  EventType const type,
  char const * const format,
  va_list varArgs
) {
  std::string str;
  str.reserve(100);

  str += '[';
  str += get_event_type_tag(type);

  char timestamp[12];
  construct_time_stamp(timestamp, sizeof timestamp);
  str += timestamp;
  str += "] ";

  char message[500];
  vsnprintf(message, sizeof message, format, varArgs);
  str += message;

  return str;
}

void log_event(EventType const type, char const * const format, ...) {
  std::scoped_lock const lock{logMutex};

  va_list varArgs;
  va_start(varArgs, format);
  std::string eventStr = construct_event_str(type, format, varArgs);
  va_end(varArgs);

  if (g_comVars.eventLogFilePathname.size() > 0) {
    std::ofstream eventLogFile(g_comVars.eventLogFilePathname, std::ios_base::app);
    if (eventLogFile.is_open()) {
      eventLogFile << eventStr << std::endl;
      eventLogFile.close();
    }
  }
}
