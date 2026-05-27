#include "WordArithConstantCache.hpp"

#include <array>

// Precomputed roots of X^64 - X + 2 with Im > 0, sorted by angle
// Obtained by running WordArithConstant::findRoots()
static constexpr std::array<std::pair<Real, Real>, HALF_BIT_WIDTH>
    PRECOMPUTED_ROOTS = {{
        {0.99886795861473276, 0.048316788945125699},
        {0.98981347811537101, 0.14457876667676595},
        {0.97171937338957637, 0.23971078871658796},
        {0.94463609664807791, 0.33291718581474672},
        {0.90867961307655942, 0.42336138330397066},
        {0.86405697909044121, 0.51017973763938884},
        {0.81108050801964016, 0.59250454249298712},
        {0.75017065528239524, 0.66948810065783781},
        {0.68185118445393633, 0.7403234241217308},
        {0.60674025204626869, 0.80426041762397116},
        {0.52553982481077077, 0.86061806802443963},
        {0.43902466005022955, 0.90879359707854068},
        {0.34803133412875442, 0.9482694107714289},
        {0.25344743954370458, 0.97861841597315913},
        {0.15620093367981172, 0.9995080461621223},
        {0.057249595063914999, 1.0107031811926872},
        {-0.042429441129934936, 1.0120680520359346},
        {-0.14185006912765974, 1.0035671707485956},
        {-0.24002749484949201, 0.98526530221623099},
        {-0.33598836844474012, 0.95732648625729755},
        {-0.42878070623629583, 0.92001211950569906},
        {-0.51748353155104809, 0.87367811200584566},
        {-0.60121615835980302, 0.8187711412997688},
        {-0.67914703934790766, 0.7558240356237762},
        {-0.75050210027828035, 0.68545032690355068},
        {-0.81457248475699917, 0.60833802310426},
        {-0.87072163736862085, 0.52524265789626146},
        {-0.9183916583128402, 0.43697968337503279},
        {-0.95710886891268354, 0.34441627861788338},
        {-0.98648853448940199, 0.24846265309255824},
        {-1.0062386989525125, 0.15006292929564893},
        {-1.0161630938959652, 0.050185693445996653},
    }};

WordArithConstant::WordArithConstant() {
  // Use precomputed roots to avoid expensive root-finding at runtime
  for (size_t i = 0; i < HALF_BIT_WIDTH; ++i)
    roots[i] = Complex(PRECOMPUTED_ROOTS[i].first, PRECOMPUTED_ROOTS[i].second);

  // You can uncomment the following line to verify that the precomputed roots
  // are correct:
  // findRoots();

  // Forward (word-to-arith) matrix: forwardMatrix[i][j] = roots[i]^j
  for (size_t i = 0; i < HALF_BIT_WIDTH; ++i) {
    Complex pow_r(1.0);
    for (size_t j = 0; j < BIT_WIDTH; ++j) {
      forwardMatrix[i][j] = pow_r;
      pow_r *= roots[i];
    }
  }

  // Backward (arith-to-word) matrix: columns are coefficients of
  // f(x) / ((x - rj) * f'(rj))

  for (size_t j = 0; j < HALF_BIT_WIDTH; ++j) {
    Complex root_j = roots[j];
    Complex inv_root_j = Complex(1.0) / root_j;

    Complex root_pow(1.0);
    Complex base = root_j;
    for (u32 exponent = BIT_WIDTH - 1; exponent > 0; exponent >>= 1) {
      if (exponent & 1)
        root_pow *= base;
      base *= base;
    }

    // f'(x) = n*x^{n-1} - 1
    Complex inv_fprime =
        Complex(1.0) /
        (Complex(static_cast<Real>(BIT_WIDTH)) * root_pow - Complex(1.0));

    // f(x)/(x - r) =
    // x^{n-1} + r*x^{n-2} + r^2*x^{n-3} + ... + r^{n-2}*x + r^{n-1} - 1
    backwardMatrix[0][j] = (root_pow - Complex(1.0)) * inv_fprime;
    for (size_t i = 1; i < BIT_WIDTH; ++i) {
      root_pow *= inv_root_j;
      backwardMatrix[i][j] = root_pow * inv_fprime;
    }
  }

  // Populate split real/imag arrays for SIMD-friendly access
  for (size_t i = 0; i < HALF_BIT_WIDTH; ++i) {
    for (size_t j = 0; j < BIT_WIDTH; ++j) {
      forwardReal[i][j] = forwardMatrix[i][j].real();
      forwardImag[i][j] = forwardMatrix[i][j].imag();
    }
  }
  for (size_t i = 0; i < BIT_WIDTH; ++i) {
    for (size_t j = 0; j < HALF_BIT_WIDTH; ++j) {
      backwardReal[i][j] = backwardMatrix[i][j].real();
      backwardImag[i][j] = backwardMatrix[i][j].imag();
    }
  }
}

void WordArithConstant::findRoots() {
  // x^n via repeated squaring in Complex128
  auto powN = [](Complex128 x) -> Complex128 {
    Complex128 result(1.0);
    Complex128 base = x;
    for (u32 exponent = BIT_WIDTH; exponent > 0; exponent >>= 1) {
      if (exponent & 1)
        result *= base;
      base *= base;
    }
    return result;
  };

  // initialize the roots for Durand-Kerner
  std::vector<Complex128> p(BIT_WIDTH);
  Real128 r = basic::pow(2.0L, 1.0 / static_cast<Real128>(BIT_WIDTH));
  Real128 angle_step = 2.0L * M_PI / BIT_WIDTH;
  for (size_t k = 0; k < BIT_WIDTH; ++k) {
    Real128 angle = angle_step * static_cast<Real128>(k) + 0.1;
    p[k] = Complex128(r * basic::cos(angle), r * basic::sin(angle));
  }

  // Durand-Kerner iteration
  for (int iter = 0; iter < 1000; ++iter) {
    Real128 max_residue = 0;
    for (size_t k = 0; k < BIT_WIDTH; ++k) {
      Complex128 denom(1.0);
      for (size_t j = 0; j < BIT_WIDTH; ++j)
        if (j != k)
          denom *= (p[k] - p[j]);
      Complex128 step = (powN(p[k]) - p[k] + Complex128(2.0)) / denom;
      p[k] -= step;
      max_residue = std::max(max_residue, basic::abs(step));
    }
    if (max_residue < 1e-30L)
      break;
  }

  // keep roots with Im > 0, sorted by angle from positive real axis
  std::vector<Complex> found;
  found.reserve(HALF_BIT_WIDTH);
  for (size_t k = 0; k < BIT_WIDTH; ++k)
    if (p[k].imag() > 0)
      found.emplace_back(static_cast<Real>(p[k].real()),
                         static_cast<Real>(p[k].imag()));

  std::sort(found.begin(), found.end(), [](const Complex &a, const Complex &b) {
    return std::atan2(a.imag(), a.real()) < std::atan2(b.imag(), b.real());
  });

  for (size_t i = 0; i < HALF_BIT_WIDTH; ++i)
    roots[i] = found[i];
}

WordArithConstant WORD_ARITH_CONSTANT;
