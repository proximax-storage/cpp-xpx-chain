/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <catapult/types.h>
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a file.
    struct File {
    public:
        /// Hash of file.
        Hash256 FileHash;
    };

    struct ReplicatorUploadInfo {
        Key Participant;
        uint64_t Uploaded;
    };

    /// Binary layout for a deleted file.
    struct DeletedFile : public File {
    public:
        /// Size of deleted file with replicators.
        uint32_t Size;

    public:
        uint16_t InfosCount() const {
            return (Size - sizeof(DeletedFile)) / sizeof(ReplicatorUploadInfo);
        }

    private:
        const uint8_t* PayloadStart() const {
            return reinterpret_cast<const uint8_t*>(this) + sizeof(DeletedFile);
        }

        uint8_t* PayloadStart() {
            return reinterpret_cast<uint8_t*>(this) + sizeof(DeletedFile);
        }

        template<typename T>
        static auto* InfosPtrT(T& file) {
            return file.InfosCount() ? file.PayloadStart() : nullptr;
        }

    public:
        // followed by replicator's upload info
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Infos, ReplicatorUploadInfo)

        bool IsSizeValid() const {
            return Size == (sizeof(DeletedFile) + InfosCount() * sizeof(ReplicatorUploadInfo));
        }
    };

    /// Binary layout for a remove action.
    struct RemoveAction : public File {
    };

    /// Binary layout for an add action.
    struct AddAction : public File {
    public:
        /// Size of file.
        uint64_t FileSize;
    };

#pragma pack(pop)
}}