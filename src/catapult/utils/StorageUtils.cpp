/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/crypto/Hashes.h"
#include "StorageUtils.h"

namespace catapult { namespace utils {

	Hash256 getEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash, const std::string& salt) {
		Hash256 eventHash;
		crypto::Sha3_256_Builder sha3;

		RawBuffer block(reinterpret_cast<const uint8_t*>(&blockTimestamp), sizeof(Timestamp));
		sha3.update({ block, utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()), generationHash });
		sha3.final(eventHash);

		return eventHash;
	}

	Hash256 getEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash, const Key& key, const std::string& salt) {
		Hash256 eventHash;
		crypto::Sha3_256_Builder sha3;

		RawBuffer block(reinterpret_cast<const uint8_t*>(&blockTimestamp), sizeof(Timestamp));
		sha3.update({ block, utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()), key, generationHash });
		sha3.final(eventHash);

		return eventHash;
	}

	Hash256 getVerificationEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash) {
		return getEventHash(blockTimestamp, generationHash, "Storage");
	}

	Hash256 getStoragePaymentEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash) {
		return getEventHash(blockTimestamp, generationHash, "Download");
	}

	Hash256 getDownloadPaymentEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash) {
		return getEventHash(blockTimestamp, generationHash, "Verification");
	}

	Hash256 getReplicatorRemovalEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash, const Key& driveKey, const Key& replicatorKey) {
		std::string salt("Replicator removal");
		Hash256 eventHash;
		crypto::Sha3_256_Builder sha3;

		RawBuffer block(reinterpret_cast<const uint8_t*>(&blockTimestamp), sizeof(Timestamp));
		sha3.update({ block, utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()), driveKey, replicatorKey, generationHash });
		sha3.final(eventHash);

		return eventHash;
	}
}}