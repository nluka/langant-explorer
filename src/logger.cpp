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

static std::string s_out_file_path{};
void logger::set_out_file_path(char const *const path) {
  s_out_file_path = path;
}
void logger::set_out_file_path(std::string const &path) {
  s_out_file_path = path;
}

static char const *s_delim = "\n";
void logger::set_delim(char const *const delim) {
  s_delim = delim;
}

static bool s_auto_flush = false;
void logger::set_autoflush(bool const b) {
  s_auto_flush = b;
}

using logger::event_type;

static char const *event_type_to_str(event_type const ev_type) {
  switch (ev_type) {
    case event_type::SIM_START:
      return "SIM_START ";
    case event_type::SAVE_PNT:
      return "SAVE_POINT";
    case event_type::SIM_END:
      return "SIM_END   ";
    case event_type::ERR:
      return "ERROR     ";

    case event_type::COUNT:
    default:
      throw std::runtime_error("bad event_type");
  }
}

class Event {
private:
  event_type const m_type;
  std::string const m_msg;
  std::chrono::system_clock::time_point const m_time_point =
      std::chrono::system_clock::now();

public:
  Event(event_type const type, char const *const msg)
    : m_type{type}, m_msg{msg} {}

  std::string stringify() const {
    std::stringstream ss{};

    ss << '[' << event_type_to_str(m_type) << ']' << ' ';

    std::time_t const time = std::chrono::system_clock::to_time_t(m_time_point);

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
static std::mutex s_events_mutex{};
#endif

static
void assert_file_opened(std::ofstream const &file) {
  if (!file) {
    std::stringstream ss{};
    ss << "failed to open file `" << s_out_file_path << '`';
    throw ss.str();
  }
}

void logger::log(event_type const ev_type, char const *const fmt, ...) {
  {
#if LOGGER_THREADSAFE
    std::scoped_lock const lock{s_events_mutex};
#endif

    va_list varArgs;
    va_start(varArgs, fmt);
    constexpr size_t len = MAX_MSG_LEN + 1; // +1 for NUL
    char msg[len];
    vsnprintf(msg, len, fmt, varArgs);
    va_end(varArgs);

    s_events.emplace_back(ev_type, msg);
  }

  if (s_auto_flush) {
    logger::flush();
  }
}

void logger::flush() {
  static bool s_file_ready = false;
  if (!s_file_ready) {
    std::ofstream file(s_out_file_path); // clear file
    assert_file_opened(file);
    s_file_ready = true;
  }

#if LOGGER_THREADSAFE
  std::scoped_lock const lock{s_events_mutex};
#endif

  if (s_events.empty()) {
    return;
  }

  std::ofstream file(s_out_file_path, std::ios_base::app);
  assert_file_opened(file);

  for (auto const &evt : s_events) {
    if (!file.good()) {
      throw "logger::flush failed - bad file";
    }
    file << evt.stringify() << s_delim;
  }

  s_events.clear();
}
