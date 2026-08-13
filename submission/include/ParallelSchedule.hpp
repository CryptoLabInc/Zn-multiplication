#pragma once

#include <algorithm>
#include <omp.h>

// Two-level OpenMP scheduling for independent ciphertext/operand tasks whose
// per-task work is itself an op-internally-parallel homomorphic computation.
template <class Count, class MakeEngines, class Body>
inline void runFheTasks(Count ntasks, int outer,
                        const MakeEngines &make_engines, const Body &body) {
  const int max_threads = omp_get_max_threads();
  const int inner = std::max(1, max_threads / std::max(1, outer));
  const bool nested = outer > 1 && inner >= 2;
  if (outer > 1) {
    if (nested)
      omp_set_max_active_levels(2);
#pragma omp parallel num_threads(outer)
    {
      if (nested)
        omp_set_num_threads(inner);
      auto engines = make_engines();
#pragma omp for schedule(dynamic)
      for (Count t = 0; t < ntasks; ++t)
        body(t, engines);
    }
  } else {
    auto engines = make_engines();
    for (Count t = 0; t < ntasks; ++t)
      body(t, engines);
  }
}

// outer-thread count for `n` independent units, capped at the hardware threads.
template <class Count> inline int outerThreads(Count n) {
  return std::max(1, std::min<int>(n, omp_get_max_threads()));
}
