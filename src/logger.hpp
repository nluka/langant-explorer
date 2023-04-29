#ifndef NLUKA_LOGGER_HPP
#define NLUKA_LOGGER_HPP

#include <string>

#define LOGGER_THREADSAFE 1 // 0 = no thread safety for `logger::write` and `logger::flush`

// Simple, threadsafe logging.
namespace logger {

// Sets the pathname of the file to write logs to.
void set_out_file_path(char const *);
// Sets the pathname of the file to write logs to.
void set_out_file_path(std::string const &);

// Sets the character sequence used to separate events. The default is "\n".
void set_delim(char const *);

// When enabled, events will be flushed after each `logger::log`. Off by default.
void set_autoflush(bool);

enum class event_type : uint8_t {
  // Simulation start
  SIM_START = 0,

  // Save point
  SAVE_PNT,

  // Simulation end
  SIM_END,

  // Error
  ERR,

  // Number of event types
  COUNT,
};

// static_assert(u8(event_type::COUNT) <= 0b01111111, "Too many logger::event_type enum members, we have 8 bits and need to reserve 1 for stdout bit.");

// Logs an event with formatted message. If `LOGGER_THREADSAFE` is non-zero, this operation is threadsafe.
void log(event_type, char const *fmt, ...);

// Flushes all buffered logs. If `LOGGER_THREADSAFE` is non-zero, this operation is threadsafe.
void flush();

} // namespace logger

#endif // NLUKA_LOGGER_HPP
