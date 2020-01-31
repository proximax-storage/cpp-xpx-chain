/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::FailedBlockHashesNotification<1>;

	DEFINE_STATELESS_VALIDATOR(FailedBlockHashes, [](const Notification& notification) {
		if (notification.BlockHashCount <= 0)
			return Failure_Service_Failed_Block_Hashes_Missing;

		std::set<Hash256> hashes;
		for (auto i = 0u; i < notification.BlockHashCount; ++i) {
			hashes.insert(notification.BlockHashesPtr[i]);
		}

		if (hashes.size() != notification.BlockHashCount)
			return Failure_Service_Duplicate_Failed_Block_Hashes;

		return ValidationResult::Success;
	});
}}
