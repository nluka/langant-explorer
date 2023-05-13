/*
  Modified version of https://github.com/nluka/cpp-logger made to integrate with term.hpp.
*/

#ifndef NLUKA_LOGGER_HPP
#define NLUKA_LOGGER_HPP

#include <string>

#include "term.hpp"
#include "primitives.hpp"

#define LOGGER_THREADSAFE 1 // 0 = no thread safety for `logger::write` and `logger::flush`

namespace logger {

  u64 const MAX_SIM_NAME_DISPLAY_LEN = 32;

  /// Sets the pathname of the file to write logs to.
  void set_out_file_path(char const *);
  /// Sets the pathname of the file to write logs to.
  void set_out_file_path(std::string const &);

  /// Sets whether to write logs to the standard output. Off by default.
  void set_stdout_logging(bool);

  /// Sets the character sequence used to separate events. The default is "\n".
  void set_delim(char const *);

  /// When enabled, events will be flushed after each `logger::log`. Off by default.
  void set_autoflush(bool);

  enum class event_type : u8 {
    SIM_START = 0,
    SIM_PROGRESS,
    SAVE_POINT,
    SIM_END,
    ERROR,
    COUNT,
  };

  /// Logs an event with formatted message. If `LOGGER_THREADSAFE` is non-zero, this operation is threadsafe.
  void log(event_type, char const *fmt, ...);

  /// Flushes all buffered logs. If `LOGGER_THREADSAFE` is non-zero, this operation is threadsafe.
  void flush();

} // namespace logger

#endif // NLUKA_LOGGER_HPP
