#pragma once

#include <chrono>

using Clock = std::chrono::high_resolution_clock;

#ifdef ENABLE_TIMER

#include <iostream>

inline double elapsed_ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}

inline void log_time(const char *label, double ms) {
  std::cout << "  [timer] " << label << ": " << ms << " ms\n";
}

#else

inline double elapsed_ms(Clock::time_point) { return 0.0; }
inline void log_time(const char *, double) {}

#endif
