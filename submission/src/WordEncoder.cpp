#include "WordEncoder.hpp"
#include "WordArithConstantCache.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <omp.h>

#define ZNMULT_THROWIF(cond)                                                   \
  do {                                                                         \
    if (cond) {                                                                \
      throw std::runtime_error("error: " #cond);                               \
    }                                                                          \
  } while (0)

#define ZNMULT_PRAGMA_OMP_PARALLEL_FOR _Pragma("omp parallel for")
#define ZNMULT_STRINGIFY_(x) #x
#define ZNMULT_STRINGIFY(x) ZNMULT_STRINGIFY_(x)
#define ZNMULT_PRAGMA_OMP_PARALLEL_FOR_ARGS(args)                              \
    _Pragma(ZNMULT_STRINGIFY(                                                   \
        omp parallel for args)) /* NOLINT(bugprone-macro-parentheses) */

inline u32 log2ceil(u64 x) { return x <= 1 ? 0 : 64 - __builtin_clzll(x - 1); }

namespace {
// inv_scales[j] = 1/(2^{j+1})
constexpr auto makeInvScales() {
  std::array<Real, BIT_WIDTH> inv_scales{};
  Real inv_scale = 0.5;
  for (u32 j = 0; j < BIT_WIDTH; ++j) {
    inv_scales[j] = inv_scale;
    inv_scale *= 0.5;
  }
  return inv_scales;
}
constexpr auto inv_scales = makeInvScales();

// masks[j] = (1 << (j+1)) - 1, except masks[BIT_WIDTH-1] = ~0
constexpr auto makeMasks() {
  std::array<u64, BIT_WIDTH> masks{};
  for (u32 j = 1; j + 1 < BIT_WIDTH; ++j)
    masks[j] = (1ULL << (j + 1)) - 1;
  masks[BIT_WIDTH - 1] = ~0ULL;
  return masks;
}
constexpr auto masks = makeMasks();

inline void toCoeffsOverT(u64 word, Real *arith) {
  arith[0] = static_cast<Real>(word) * inv_scales[BIT_WIDTH - 1] -
             static_cast<Real>(word & 1) * 0.5;
  for (size_t j = 1; j < BIT_WIDTH; ++j)
    arith[j] = -static_cast<Real>(word & masks[j]) * inv_scales[j];
}

inline void forwardMatvec(const Real *__restrict a, Real *__restrict out_re,
                          Real *__restrict out_im) {
  const auto &forwardReal = WORD_ARITH_CONSTANT.forwardReal;
  const auto &forwardImag = WORD_ARITH_CONSTANT.forwardImag;
  for (size_t i = 0; i < HALF_BIT_WIDTH; ++i)
    out_re[i] = out_im[i] = 0;
  for (size_t j = 0; j < BIT_WIDTH; ++j) {
    Real aj = a[j];
    const Real *fr = forwardReal[j].data();
    const Real *fi = forwardImag[j].data();
    for (size_t i = 0; i < HALF_BIT_WIDTH; ++i) {
      out_re[i] += fr[i] * aj;
      out_im[i] += fi[i] * aj;
    }
  }
}

inline void backwardMatvec(const Real *__restrict re, const Real *__restrict im,
                           Real *__restrict out) {
  const auto &backwardReal = WORD_ARITH_CONSTANT.backwardReal;
  const auto &backwardImag = WORD_ARITH_CONSTANT.backwardImag;
  for (size_t i = 0; i < BIT_WIDTH; ++i)
    out[i] = 0;
  for (size_t j = 0; j < HALF_BIT_WIDTH; ++j) {
    Real mr = re[j], mi = im[j];
    const Real *br = backwardReal[j].data();
    const Real *bi = backwardImag[j].data();
    for (size_t i = 0; i < BIT_WIDTH; ++i)
      out[i] += br[i] * mr - bi[i] * mi;
  }
}
} // namespace

WordEncodeParams &WordEncodeParams::setLogDegree(u32 log_degree) {
  this->log_degree_ = log_degree;
  return *this;
}

WordEncodeParams &WordEncodeParams::setBatchSize(u32 batch_size) {
  this->batch_size_ = batch_size;
  return *this;
}

WordEncoder::WordEncoder(const WordEncodeParams &params) : params_(params) {}

void WordEncoder::wordToArith(const std::vector<u64> &words,
                              Message &arith) const {
  ZNMULT_THROWIF(words.empty());
  auto num_words = static_cast<u32>(words.size());
  auto log_slots = log2ceil(static_cast<u64>(BIT_WIDTH) * num_words);

  arith = Message(log_slots);
  ZNMULT_PRAGMA_OMP_PARALLEL_FOR
  for (size_t i = 0; i < num_words; ++i) {
    u64 word = words[i];
    size_t base = i * BIT_WIDTH;

    arith[base] = static_cast<Real>(word) * inv_scales[BIT_WIDTH - 1] -
                  static_cast<Real>(word & 1) * 0.5;
    for (size_t j = 1; j < BIT_WIDTH; ++j) {
      Real lower = static_cast<Real>(word & masks[j]) * inv_scales[j];
      arith[base + j] = -lower;
    }
  }
}

void WordEncoder::arithToWord(const Message &arith,
                              std::vector<u64> &words) const {
  size_t num_words = (1UL << arith.logSlots()) / BIT_WIDTH;

  words.resize(num_words);
  Real tmp[BIT_WIDTH];
  ZNMULT_PRAGMA_OMP_PARALLEL_FOR_ARGS(firstprivate(tmp))
  for (size_t w = 0; w < num_words; ++w) {
    size_t base = w * BIT_WIDTH;
    for (size_t i = 0; i + 1 < BIT_WIDTH; ++i)
      tmp[i + 1] = arith[base + i].real() - 2.0 * arith[base + i + 1].real();
    Real leading_coeff = arith[base + BIT_WIDTH - 1].real();
    tmp[0] = -2.0 * (leading_coeff + arith[base].real());
    tmp[1] += leading_coeff;

    u64 word = 0;
    for (size_t i = 0; i < BIT_WIDTH; ++i) {
      const i32 coeff = static_cast<i32>(std::lrint(2.0 * tmp[i]));
      word += static_cast<u64>(coeff) << i;
    }

    words[w] = word;
  }
}

void WordEncoder::arithToComplex(const Message &arith, Message &msg) const {
  auto num_words = (1UL << arith.logSlots()) / BIT_WIDTH;
  auto per_batch_complex_slots = num_words * HALF_BIT_WIDTH;
  auto per_batch_log_slots = log2ceil(per_batch_complex_slots);

  msg = Message(per_batch_log_slots);
  ZNMULT_PRAGMA_OMP_PARALLEL_FOR
  for (size_t w = 0; w < num_words; ++w) {
    const Complex *arith_ptr = &arith[w * BIT_WIDTH];
    Real arith_buf[BIT_WIDTH];
    for (size_t j = 0; j < BIT_WIDTH; ++j)
      arith_buf[j] = arith_ptr[j].real();

    Real real_acc[HALF_BIT_WIDTH], imag_acc[HALF_BIT_WIDTH];
    forwardMatvec(arith_buf, real_acc, imag_acc);

    Complex *msg_ptr = &msg[w * HALF_BIT_WIDTH];
    for (size_t i = 0; i < HALF_BIT_WIDTH; ++i)
      msg_ptr[i] = Complex(real_acc[i], imag_acc[i]);
  }
}

void WordEncoder::complexToArith(const Message &msg, Message &ariths) const {
  auto log_slots = msg.logSlots();
  auto num_slots = 1UL << log_slots;
  auto num_words = num_slots / HALF_BIT_WIDTH;

  ariths = Message(log_slots + 1);
  ZNMULT_PRAGMA_OMP_PARALLEL_FOR
  for (size_t w = 0; w < num_words; ++w) {
    const Complex *msg_ptr = &msg[w * HALF_BIT_WIDTH];
    Real msg_re[HALF_BIT_WIDTH], msg_im[HALF_BIT_WIDTH];
    for (size_t j = 0; j < HALF_BIT_WIDTH; ++j) {
      msg_re[j] = msg_ptr[j].real();
      msg_im[j] = msg_ptr[j].imag();
    }

    Real acc[BIT_WIDTH];
    backwardMatvec(msg_re, msg_im, acc);

    Complex *arith_ptr = &ariths[w * BIT_WIDTH];
    for (size_t i = 0; i < BIT_WIDTH; ++i)
      arith_ptr[i] = acc[i];
  }
}

void WordEncoder::wordToComplex(const std::vector<std::vector<u64>> &words,
                                std::vector<Message> &msgs) const {
  ZNMULT_THROWIF(!params_.has_value());
  auto batch_size = params_->batch_size_;

  ZNMULT_THROWIF(words.empty());
  auto num_words = static_cast<u32>(words[0].size());
  for (const auto &word : words)
    ZNMULT_THROWIF(word.size() != num_words);

  auto num_slots = num_words * HALF_BIT_WIDTH;
  auto log_slots = log2ceil(num_slots);

  msgs.resize(batch_size);
  for (auto &m : msgs)
    m = Message(log_slots);

  ZNMULT_PRAGMA_OMP_PARALLEL_FOR_ARGS(collapse(2))
  for (size_t b = 0; b < batch_size; ++b) {
    for (size_t w = 0; w < num_words; ++w) {
      Real arith_buf[BIT_WIDTH];
      toCoeffsOverT(words[b][w], arith_buf);

      Real real_acc[HALF_BIT_WIDTH], imag_acc[HALF_BIT_WIDTH];
      forwardMatvec(arith_buf, real_acc, imag_acc);

      Complex *msg_ptr = &msgs[b][w * HALF_BIT_WIDTH];
      for (size_t i = 0; i < HALF_BIT_WIDTH; ++i)
        msg_ptr[i] = Complex(real_acc[i], imag_acc[i]);
    }
  }
}

void WordEncoder::complexToWord(const std::vector<Message> &msgs,
                                std::vector<std::vector<u64>> &words) const {
  ZNMULT_THROWIF(!params_.has_value());
  auto batch_size = params_->batch_size_;

  ZNMULT_THROWIF(msgs.size() != batch_size);
  auto log_slots = msgs[0].logSlots();
  for (const auto &msg : msgs)
    ZNMULT_THROWIF(msg.logSlots() != log_slots);
  auto num_slots = 1UL << log_slots;

  auto num_words = num_slots / HALF_BIT_WIDTH;

  words.resize(batch_size);
  for (auto &batch : words)
    batch.resize(num_words);

  ZNMULT_PRAGMA_OMP_PARALLEL_FOR_ARGS(collapse(2))
  for (size_t b = 0; b < batch_size; ++b) {
    for (size_t w = 0; w < num_words; ++w) {
      const Complex *msg_ptr = &msgs[b][w * HALF_BIT_WIDTH];
      Real msg_re[HALF_BIT_WIDTH], msg_im[HALF_BIT_WIDTH];
      for (size_t j = 0; j < HALF_BIT_WIDTH; ++j) {
        msg_re[j] = msg_ptr[j].real();
        msg_im[j] = msg_ptr[j].imag();
      }

      Real tmp[BIT_WIDTH];
      backwardMatvec(msg_re, msg_im, tmp);

      // arithToWord
      Real coeffs[BIT_WIDTH];
      for (size_t i = 0; i + 1 < BIT_WIDTH; ++i)
        coeffs[i + 1] = tmp[i] - 2.0 * tmp[i + 1];
      Real leading_coeff = tmp[BIT_WIDTH - 1];
      coeffs[0] = -2.0 * (leading_coeff + tmp[0]);
      coeffs[1] += leading_coeff;

      u64 word = 0;
      for (size_t i = 0; i < BIT_WIDTH; ++i) {
        const i32 coeff = static_cast<i32>(std::lrint(2.0 * coeffs[i]));
        word += static_cast<u64>(coeff) << i;
      }

      words[b][w] = word;
    }
  }
}
