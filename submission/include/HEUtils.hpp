#pragma once

#include "HEaaN2/HEaaN2.hpp"

using namespace heaan;

static constexpr u32 BIT_WIDTH = 64;
static constexpr PresetParamsId HARNESS_PRESET = PresetParamsId::S16_Gr;

inline ParamsSet
getParamsSet(const std::string &instance,
             std::optional<PresetParamsId> preset_id = std::nullopt) {
  if (preset_id.has_value())
    return makePresetParamsSet(preset_id.value());

  // max_secure_bits is obtained from
  // https://github.com/jdumezy/sparse-key-estimate/blob/master/Precomputed-Tables/128bits_security.md
  // It is used to determine the gadget decomposition structure of the
  // relinearization key, and does not directly correspond to the modulus of
  // the ciphertext. In practice, the actual modulus used in the parameter can
  // be much smaller than max_secure_bits, depending on the specific parameters
  // chosen for the scheme. For example, the max log2(PQ) of the following
  // parameters is equal or less than 114.
  u32 log_degree, hw, max_secure_bits;
  if (instance == "single") {
    log_degree = 12;
    hw = 0; // hw = 0 means uniform ternary
    max_secure_bits = 106;
  } else if (instance == "small" || instance == "medium" ||
             instance == "large") {
    log_degree = 16;
    hw = 32;
    max_secure_bits = 328;
  } else {
    throw std::runtime_error("unknown instance: " + instance);
  }

  // With PolyType::GRAFTED, we are using the grafting technique,
  // introduced in https://eprint.iacr.org/2024/1014. This allows more flexible
  // modulus management.
  auto poly_type = PolyType::GRAFTED;
  auto dist = DiscreteGaussian(3.2);

  // Secret Key Generation Parameters
  SKGenParams skgen_params(log_degree, hw);

  // Modulus Chain Construction
  u32 base_modulus_bits = (log_degree == 12) ? 20 : 25;
  paramsUtils::LevelsBuilder levels_builder;
  levels_builder.setRing(log_degree, poly_type, false);
  levels_builder.initMod(base_modulus_bits);
  auto eval_levels = levels_builder.buildAbove(2, 15);
  auto &ecd_mod = eval_levels.mods.back();

  // Encryption Key Generation Parameters
  EncKeyGenParams ekgen_params(dist, poly_type, ecd_mod);

  // Switching Key Generation Parameters
  paramsUtils::SwKeyGenParamsBuilder swkgen_builder;
  swkgen_builder.setRing(log_degree, poly_type);
  swkgen_builder.setModUpPrimes(max_secure_bits, 0.0);
  auto swkgen_params = swkgen_builder.build(ecd_mod);

  // Encoding Parameters
  EncodeParams ecd_params(poly_type, log_degree, eval_levels);

  // Encryption Parameters
  EncryptParams enc_params(dist);

  // Homomorphic Evaluation Parameters
  HomEvalParams eval_params(eval_levels);

  return {skgen_params, ekgen_params, swkgen_params,
          ecd_params,   enc_params,   eval_params};
}

inline u32 computeNumWords(const ParamsSet &params) {
  u32 log_degree = params.ecd_params.getLogDegree();
  u32 degree = 1U << log_degree;
  return degree / BIT_WIDTH;
}

inline u32 computeNumCiphertexts(u64 num_target_words, u32 num_words,
                                 u32 batch_size) {
  if (num_target_words <= num_words)
    return 1;

  u64 words_per_ct = static_cast<u64>(num_words) * batch_size;
  return static_cast<u32>((num_target_words + words_per_ct - 1) / words_per_ct);
}

inline PtxtType getPtxtType(u32 batch_size) {
  return batch_size == 1 ? PtxtType::NORMAL : PtxtType::BATCH;
}

inline EncType getEncType(u32 batch_size) {
  return batch_size == 1 ? EncType::RLWE : EncType::BatchRLWE;
}

inline void encodeBatch(const EnDecoder &enc, const std::vector<Message> &msgs,
                        IPlaintext &ptxt, u32 level) {
  if (msgs.size() == 1)
    enc.encode(msgs[0], ptxt, level);
  else
    enc.encode(msgs, ptxt, level);
}

inline void decodeBatch(const EnDecoder &enc, const IPlaintext &ptxt,
                        std::vector<Message> &msgs, u32 batch_size) {
  msgs.resize(batch_size);
  if (batch_size == 1)
    enc.decode(ptxt, msgs[0]);
  else
    enc.decode(ptxt, msgs);
}
