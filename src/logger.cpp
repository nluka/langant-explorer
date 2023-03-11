#include <chrono>
#include <cstdarg>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

#include "logger.hpp"

// CONFIGURATION:
#define MAX_MSG_LEN 1000

static std::string s_outPathname{};
void logger::set_out_pathname(char const *const pathname) {
  s_outPathname = pathname;
}
void logger::set_out_pathname(std::string const &pathname) {
  s_outPathname = pathname;
}

static char const *s_delim = "\n";
void logger::set_delim(char const *const delim) {
  s_delim = delim;
}

static bool s_autoFlush = false;
void logger::set_autoflush(bool const b) {
  s_autoFlush = b;
}

using logger::event_type;

static
char const *event_type_to_str(event_type const evType) {
  switch (evType) {
    case event_type::SIM_START: return "SIM_START ";
    case event_type::SAVE_PNT:  return "SAVE_POINT";
    case event_type::SIM_END:   return "SIM_END   ";
    case event_type::ERR:       return "ERROR     ";
    default: throw std::runtime_error("bad event_type");
  }
}

class Event {
private:
  event_type const m_type;
  std::string const m_msg;
  std::chrono::system_clock::time_point const m_timepoint =
    std::chrono::system_clock::now();

public:
  Event(event_type const type, char const *const msg)
    : m_type{type}, m_msg{msg} {}

  std::string stringify() const {
    std::stringstream ss{};

    ss << '[' << event_type_to_str(m_type) << ']' << ' ';

    std::time_t const time = std::chrono::system_clock::to_time_t(m_timepoint);

    // timestamp
    #if 0
    char const *const raw_time_stamp = ctime(&time); // has a \n at the end
    std::string_view time_stamp(raw_time_stamp, std::strlen(raw_time_stamp) - 1);
    ss << '(' << time_stamp << ')' << ' ';
    #else
    {
      struct tm const *local = localtime(&time);

      ss << '('
        << (local->tm_year + 1900) << '-'
        << local->tm_mon << '-'
        << local->tm_mday << ' '
        << std::setfill('0')
        << local->tm_hour << ':'
        << std::setw(2) << local->tm_min << ':'
        << std::setw(2) << local->tm_sec
      << ") ";
    }
    #endif

    ss << m_msg;

    return ss.str();
  }
};

static std::vector<Event> s_events{};
#if LOGGER_THREADSAFE
static std::mutex s_eventsMutex{};
#endif

static
void assert_file_opened(std::ofstream const &file) {
  if (!file.is_open()) {
    std::stringstream ss{};
    ss << "failed to open file `" << s_outPathname << '`';
    throw ss.str();
  }
}

void logger::log(event_type const evType, char const *const fmt, ...) {
  {
    #if LOGGER_THREADSAFE
    std::scoped_lock const lock{s_eventsMutex};
    #endif

    va_list varArgs;
    va_start(varArgs, fmt);
    constexpr size_t len = MAX_MSG_LEN + 1; // +1 for NUL
    char msg[len];
    vsnprintf(msg, len, fmt, varArgs);
    va_end(varArgs);

    s_events.emplace_back(evType, msg);
  }

  if (s_autoFlush) {
    logger::flush();
  }
}

void logger::flush() {
  static bool s_isFileReady = false;
  if (!s_isFileReady) {
    std::ofstream file(s_outPathname); // clear file
    assert_file_opened(file);
    s_isFileReady = true;
  }

  #if LOGGER_THREADSAFE
  std::scoped_lock const lock{s_eventsMutex};
  #endif

  if (s_events.empty()) {
    return;
  }

  std::ofstream file(s_outPathname, std::ios_base::app);
  assert_file_opened(file);

  for (auto const &evt : s_events) {
    if (!file.good()) {
      throw "logger::flush failed - bad file";
    }
    file << evt.stringify() << s_delim;
  }

  s_events.clear();
}
