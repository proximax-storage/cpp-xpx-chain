/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/SuperContractCache.h"
#include "src/model/SuperContractEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::SuperContractNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(SuperContract, [](const Notification& notification, const ValidatorContext& context) {
		const auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();

		if (!superContractCache.contains(notification.SuperContract))
			return Failure_SuperContract_SuperContract_Does_Not_Exist;

		auto superContractCacheIter = superContractCache.find(notification.SuperContract);
		auto& superContractEntry = superContractCacheIter.get();

		static std::unordered_set<model::EntityType> blockedTransactionsAfterDeactivation({
		    model::Entity_Type_StartExecute,
		    model::Entity_Type_Deactivate,
		    model::Entity_Type_Suspend,
		    model::Entity_Type_Resume,
		});

		if (superContractEntry.state() > state::SuperContractState::Suspended &&
			blockedTransactionsAfterDeactivation.count(notification.TransactionType))
				return Failure_SuperContract_Operation_Is_Not_Permitted;

		return ValidationResult::Success;
	});
}}
