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
            /// Index of Prover from Provers array.
            uint16_t Verifier;

            /// Signatures of opinions.
            catapult::Signature Signature;

            /// Opinion about verification results for each Prover. Success or Failure.
            std::vector<std::pair<uint16_t , uint8_t>> Results;
        };

#pragma pack(pop)
}}