/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/CatapultConfigCache.h"

namespace catapult { namespace validators {

	using Notification = model::CatapultBlockChainConfigNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(CatapultConfig, Notification)(uint32_t maxBlockChainConfigSize) {
		return MAKE_STATEFUL_VALIDATOR(CatapultConfig, ([maxBlockChainConfigSize](const Notification& notification, const ValidatorContext& context) {
			if (notification.BlockChainConfigSize > maxBlockChainConfigSize)
				return Failure_CatapultConfig_BlockChain_Config_Too_Large;

			const auto& cache = context.Cache.sub<cache::CatapultConfigCache>();
			if (cache.find(context.Height).tryGet())
				return Failure_CatapultConfig_Redundant;

			return ValidationResult::Success;
		}));
	}
}}
