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

#include "HEaaN2/EnDecoder.hpp"
#include "HEaaN2/EnDecryptor.hpp"
#include "HEaaN2/EncKeyGenerator.hpp"
#include "HEaaN2/HomEval.hpp"
#include "HEaaN2/PresetParams.hpp"
#include "HEaaN2/SKGenerator.hpp"
#include "HEaaN2/SwKeyGenerator.hpp"

namespace heaan {

/// @brief A set of parameters for various modules
struct ParamsSet {
    SKGenParams skgen_params;
    EncKeyGenParams ekgen_params;
    SwKeyGenParams swkgen_params;
    EncodeParams ecd_params;
    EncryptParams enc_params;
    HomEvalParams eval_params;
};

/// @brief Creates a preset ParamsSet
/// @param id PresetParamsId
/// @return The corresponding ParamsSet
ParamsSet HEAAN2_API makePresetParamsSet(PresetParamsId id);

} // namespace heaan
