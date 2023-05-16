#ifndef NLUKA_SCOPEDTIMER_HPP
#define NLUKA_SCOPEDTIMER_HPP

#include <chrono>
#include <cstdint>
#include <ostream>
#include <string_view>

namespace scoped_timer_unit
{
  typedef uint32_t value_type;

  value_type constexpr
    SECONDS      = 1'000'000'000,
    MILLISECONDS = 1'000'000,
    MICROSECONDS = 1'000,
    NANOSECONDS  = 1;
};

template <scoped_timer_unit::value_type TimerUnit>
class scoped_timer
{
  public:
    scoped_timer(char const *const label, std::ostream &os)
    : m_label{label},
      m_os{os},
      m_start{std::chrono::steady_clock::now()}
    {}

    scoped_timer(scoped_timer const &) = delete; // copy constructor
    scoped_timer &operator=(scoped_timer const &) = delete; // copy assignment
    scoped_timer(scoped_timer &&) noexcept = delete; // move constructor
    scoped_timer &operator=(scoped_timer &&) noexcept = delete; // move assignment

    ~scoped_timer() {
      auto const end = std::chrono::steady_clock::now();
      auto const elapsed_nanos = end - m_start;
      auto const elapsed_time_in_units = static_cast<double>(elapsed_nanos.count() / TimerUnit);

      char const *unit_cstr;
      switch (TimerUnit) {
        case scoped_timer_unit::SECONDS:      unit_cstr = "s"; break;
        case scoped_timer_unit::MILLISECONDS: unit_cstr = "ms"; break;
        case scoped_timer_unit::MICROSECONDS: unit_cstr = "us"; break;
        case scoped_timer_unit::NANOSECONDS:  unit_cstr = "ns"; break;
        default:                              unit_cstr = nullptr; break;
      }

      m_os << m_label << " took " << elapsed_time_in_units << unit_cstr << '\n';
    }

  private:
    std::string_view const m_label;
    std::ostream &m_os;
    std::chrono::time_point<std::chrono::steady_clock> const m_start;
};

#endif // NLUKA_SCOPEDTIMER_HPP
