#pragma once

#include <chrono>
#include <iostream>

using Clock = std::chrono::high_resolution_clock;

inline double elapsed_ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}

inline void log_time(const char *label, double ms) {
  std::cout << "\t\t" << label << ": " << ms << " ms\n";
}
