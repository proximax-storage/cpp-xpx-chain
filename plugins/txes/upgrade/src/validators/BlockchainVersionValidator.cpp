/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/version/version.h"
#include "plugins/txes/upgrade/src/cache/BlockchainUpgradeCache.h"

namespace catapult { namespace validators {

	using Notification = model::BlockNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(BlockchainVersion, [](const auto&, const ValidatorContext& context) {
		const auto& cache = context.Cache.sub<cache::BlockchainUpgradeCache>();
		auto iter = cache.find(context.Height);
	  	const auto& entry = iter.tryGet();
		if (entry && entry->blockChainVersion() > version::CurrentBlockchainVersion)
			return Failure_BlockchainUpgrade_Invalid_Current_Version;

		return ValidationResult::Success;
	});
}}
