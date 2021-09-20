/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

        /// Binary layout of a verification opinion.
        struct VerificationOpinion {
            /// Public Keys of the Verifier.
            Key Verifier;

            /// Aggregated BLS signatures of opinions.
            BLSSignature BlsSignature;

            /// Opinion about verification status for each Prover. Success or Failure.
            std::vector<std::pair<Key, uint8_t>> Opinions;
        };

#pragma pack(pop)
}}