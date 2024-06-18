/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <catapult/types.h>

namespace catapult { namespace utils {

	Hash256 getVerificationEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash);
	Hash256 getStoragePaymentEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash);
	Hash256 getDownloadPaymentEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash);
	Hash256 getReplicatorRemovalEventHash(const Timestamp& blockTimestamp, const catapult::GenerationHash& generationHash, const Key& driveKey, const Key& replicatorKey);
}}