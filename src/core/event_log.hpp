#ifndef ANT_SIMULATOR_LITE_CORE_LOG_EVENT_HPP
#define ANT_SIMULATOR_LITE_CORE_LOG_EVENT_HPP

#include <stdbool.h>

enum class EventType {
  Info,
  Success,
  Warning,
  Error
};

void prepare_event_log_file();

void log_event(EventType type, char const * const format, ...);

#endif // ANT_SIMULATOR_LITE_CORE_LOG_EVENT_HPP
