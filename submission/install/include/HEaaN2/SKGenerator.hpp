////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Copyright (C) 2025-2026 Crypto Lab Inc.                                    //
//                                                                            //
// - This file is a part of HEaaN2 homomorphic encryption library.            //
// - This header is provided for use with the HEaaN2 library and may be       //
//   included in software that links against HEaaN2.                          //
// - Redistribution or modification of this file, in whole or in part,        //
//   is not permitted without prior written consent from Crypto Lab Inc.      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "HEaaN2/General.hpp"
#include "HEaaN2/ISecretKey.hpp"
#include "HEaaN2/PolyRing.hpp"
#include "HEaaN2/PresetParams.hpp"

namespace heaan {

/// @brief A struct for secret key generation parameters
/// @details
/// - log_degree: log degree of the ring
/// - hw: hamming weight of the secret key
struct HEAAN2_API SKGenParams {
public:
    /// @brief Constructs SKGenParams with the specified parameters.
    /// @param log_degree log_degree of the secret key.
    /// @param hw hamming weight of the secret key.
    /// @param ntt_alg The NTT algorithm (NORMAL/CYC_FOR_CI) used for the secret
    /// key generation.
    SKGenParams(u32 log_degree, u32 hw,
                NTTAlgorithm ntt_alg = NTTAlgorithm::NORMAL)
        : log_degree_(log_degree), hw_(hw), ntt_alg_(ntt_alg) {}

    /// @brief Gets the log_degree of the secret key.
    /// @return log_degree of the secret key.
    u32 getLogDegree() const { return log_degree_; }
    /// @brief Gets the hamming weight of the secret key.
    /// @return hamming weight of the secret key.
    u32 getHammingWeight() const { return hw_; }
    /// @brief Gets the NTT algorithm.
    /// @return The NTT algorithm.
    NTTAlgorithm getNTTAlgorithm() const { return ntt_alg_; };

private:
    u32 log_degree_;
    u32 hw_;
    NTTAlgorithm ntt_alg_;
};

/// @brief Secret key generator
class HEAAN2_API SKGenerator {
public:
    /// @brief Constructs a SKGenerator with the specified preset parameters.
    /// @param id Preset parameters identifier.
    SKGenerator(PresetParamsId id);

    /// @brief Constructs a SKGenerator with the specified secret key generation
    /// parameters.
    /// @param params Secret key generation parameters.
    SKGenerator(const SKGenParams &params);

    /// @brief Generates a secret key
    /// @return The pointer to the secret key.
    /// @details The device of the output is always on CPU.
    Ptr<ISecretKey> genKey() const;

    const SKGenParams &getParams() const { return params_; }

private:
    Pimpl impl;
    SKGenParams params_;
};

} // namespace heaan
