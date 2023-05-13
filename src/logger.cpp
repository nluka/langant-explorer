#include <chrono>
#include <cstdarg>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>
#include <iostream>
#include <optional>

#include "term.hpp"
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

static bool s_write_to_stdout = false;
void logger::set_stdout_logging(bool const b) {
  s_write_to_stdout = b;
}

using logger::event_type;

static char const *event_type_to_str(event_type const ev_type) {
  switch (ev_type) {
    case event_type::SIM_START:    return "SIM_START   ";
    case event_type::SIM_PROGRESS: return "SIM_PROGRESS";
    case event_type::SAVE_POINT:   return "SAVE_POINT  ";
    case event_type::SIM_END:      return "SIM_END     ";
    case event_type::ERROR:        return "ERROR       ";

    case event_type::COUNT:
    default:
      throw std::runtime_error("bad event_type");
  }
}

class log_event {
private:
  std::string const m_msg;
  std::chrono::system_clock::time_point const m_time_point = std::chrono::system_clock::now();
  unsigned int m_color;
  event_type const m_type;

public:
  log_event(event_type const type, unsigned int const color, char const *const msg)
    : m_msg{msg}
    , m_color{color}
    , m_type{type}
  {}

  [[nodiscard]] unsigned int font_effects() const noexcept { return m_color; };

  std::string stringify() const {
    static std::stringstream ss{};

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

    std::string retval = ss.str();

    // reset `ss`
    ss.str("");
    ss.seekp(0);

    return retval;
  }
};

static std::vector<log_event> s_events{};
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
  static constexpr
  term::font_effects_t s_event_type_to_font_effects_map[u64(event_type::COUNT)] {
    0,
    0,
    term::FG_YELLOW,
    term::FG_GREEN,
    term::FG_RED,
  };

  if (s_out_file_path == "" && !s_write_to_stdout) {
    return;
  }

  {
#if LOGGER_THREADSAFE
    std::scoped_lock const lock{s_events_mutex};
#endif

    constexpr size_t len = MAX_MSG_LEN + 1; // +1 for NUL
    static char msg[len];

    va_list var_args;
    va_start(var_args, fmt);
    vsnprintf(msg, len, fmt, var_args);
    va_end(var_args);

    term::font_effects_t const effects = s_event_type_to_font_effects_map[u64(ev_type)];
    s_events.emplace_back(ev_type, effects, msg);
  }

  if (s_auto_flush) {
    logger::flush();
  }
}

void logger::flush() {
#if LOGGER_THREADSAFE
  std::scoped_lock const lock{s_events_mutex};
#endif

  static bool s_file_ready = false;

  if (s_out_file_path != "" && !s_file_ready) {
    std::ofstream file(s_out_file_path); // clear file
    assert_file_opened(file);
    s_file_ready = true;
  }

  if (s_events.empty()) {
    return;
  }

  std::optional<std::ofstream> file = std::nullopt;
  if (s_out_file_path != "") {
    file = std::ofstream(s_out_file_path, std::ios_base::app);
    assert_file_opened(file.value());
  }

  for (auto const &evt : s_events) {
    if (!file) {
      throw std::runtime_error("logger::flush failed - bad file");
    }

    std::string const event_str = evt.stringify();

    if (file.has_value())
      file.value() << event_str << s_delim;
    if (s_write_to_stdout)
      term::printf(evt.font_effects(), "%s%s", event_str.c_str(), s_delim);
  }

  s_events.clear();
}
