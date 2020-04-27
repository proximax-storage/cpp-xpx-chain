/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace validators {

		using Notification = model::ResumeNotification<1>;

		DEFINE_STATEFUL_VALIDATOR(Resume, [](const Notification& notification, const ValidatorContext& context) {
			const auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
			auto superContractCacheIter = superContractCache.find(notification.SuperContract);
			auto& superContractEntry = superContractCacheIter.get();

			if (superContractEntry.owner() != notification.Signer)
				return Failure_SuperContract_Operation_Is_Not_Permitted;

			if (superContractEntry.state() != state::SuperContractState::Suspended)
				return Failure_SuperContract_Operation_Is_Not_Permitted;

			return ValidationResult::Success;
		});
	}}
