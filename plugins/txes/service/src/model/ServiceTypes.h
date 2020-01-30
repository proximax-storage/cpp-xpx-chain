/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout of a file.
    struct File {
    public:
        /// Hash of file.
        Hash256 FileHash;

	public:
		bool operator==(const File& other) const {
			return other.FileHash == this->FileHash;
		}
    };

    struct UploadInfo {
        Key Participant;
        uint64_t Uploaded;
    };

    /// Binary layout of an add action.
    struct AddAction : public File {
    public:
        /// Size of file.
        uint64_t FileSize;

	public:
		bool operator==(const AddAction& other) const {
			return other.FileSize == this->FileSize && static_cast<const File&>(*this) == static_cast<const File&>(other);
		}
    };

    /// Binary layout of a remove action.
    struct RemoveAction : public AddAction {
    };

    /// Binary layout of failed verification data.
    struct VerificationFailure : public SizePrefixedEntity {
    public:
        /// The replicator that failed verification.
        Key Replicator;

    public:
		/// Count of the failed block hashes.
        uint16_t BlockHashCount() const {
            return (Size - sizeof(VerificationFailure)) / sizeof(Hash256);
        }

	private:
		const uint8_t* PayloadStart() const {
			return reinterpret_cast<const uint8_t*>(this) + sizeof(VerificationFailure);
		}

		uint8_t* PayloadStart() {
			return reinterpret_cast<uint8_t*>(this) + sizeof(VerificationFailure);
		}

		template<typename T>
		static auto* BlockHashesPtrT(T& failure) {
			return failure.BlockHashCount() ? failure.PayloadStart() : nullptr;
		}

	public:
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(BlockHashes, Hash256)

		bool IsSizeValid() const {
			return Size == (sizeof(VerificationFailure) + BlockHashCount() * sizeof(Hash256));
		}
    };

#pragma pack(pop)
}}