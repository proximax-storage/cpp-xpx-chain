/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include <catapult/types.h>
#include <catapult/crypto/Hashes.h>
#include "StorageUtils.h"

namespace catapult { namespace utils {
	Hash256 getEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash, const std::string& salt) {
		Hash256 eventHash;
		crypto::Sha3_256_Builder sha3;

		RawBuffer block(reinterpret_cast<const uint8_t *>(&blockTimestamp), sizeof(Timestamp));
		sha3.update({block,
					 utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()),
					 generationHash});
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
}}