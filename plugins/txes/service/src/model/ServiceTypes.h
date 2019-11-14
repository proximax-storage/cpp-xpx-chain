/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout of a file.
    struct File {
    public:
        /// Hash of file.
        Hash256 FileHash;
    };

    /// Binary layout of a remove action.
    struct RemoveAction : public File {
    };

    /// Binary layout of an add action.
    struct AddAction : public File {
    public:
        /// Size of file.
        uint64_t FileSize;
    };

    /// Binary layout of failed verification data.
    struct VerificationFailure {
    public:
        /// The replicator that failed verification.
        Key Replicator;

        /// The hash of the failed block.
        Hash256 BlockHash;
    };

#pragma pack(pop)
}}