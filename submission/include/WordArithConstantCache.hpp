#pragma once

#include "HEUtils.hpp"

#include <array>
static constexpr u32 HALF_BIT_WIDTH = BIT_WIDTH / 2;

class WordArithConstant {
public:
  WordArithConstant();

  // roots of X^N - X + 2 with Im > 0, sorted by angle from positive real axis
  std::array<Complex, HALF_BIT_WIDTH> roots;

  // (N/2) x N Vandermonde matrix (Complex)
  std::array<std::array<Complex, BIT_WIDTH>, HALF_BIT_WIDTH> forwardMatrix;
  // N x (N/2) complex matrix (Complex, for non-fused path)
  std::array<std::array<Complex, HALF_BIT_WIDTH>, BIT_WIDTH> backwardMatrix;

  std::array<std::array<Real, HALF_BIT_WIDTH>, BIT_WIDTH> forwardReal;
  std::array<std::array<Real, HALF_BIT_WIDTH>, BIT_WIDTH> forwardImag;
  std::array<std::array<Real, BIT_WIDTH>, HALF_BIT_WIDTH> backwardReal;
  std::array<std::array<Real, BIT_WIDTH>, HALF_BIT_WIDTH> backwardImag;

  void findRoots();
};

extern WordArithConstant WORD_ARITH_CONSTANT;
