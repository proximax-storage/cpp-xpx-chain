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

#include "Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"

namespace catapult { namespace validators {

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(NewRemoteAccountAvailability, model::NewRemoteAccountNotification<1>, [](const model::NewRemoteAccountNotification<1>& notification, const auto& context) {
		const auto& cache = context.Cache.template sub<cache::AccountStateCache>();
		auto remoteAccountStateIter = cache.find(notification.RemoteAccountKey);
		return !remoteAccountStateIter.tryGet() || state::AccountType::Remote_Unlinked == remoteAccountStateIter.get().AccountType
				? ValidationResult::Success
				: Failure_AccountLink_Remote_Account_Ineligible;
	});
	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(NewRemoteAccountAvailabilityV2, model::NewRemoteAccountNotification<2>, [](const model::NewRemoteAccountNotification<2>& notification, const auto& context) {
		const auto& cache = context.Cache.template sub<cache::AccountStateCache>();
		auto remoteAccountStateIter = cache::FindAccountStateByPublicKeyOrAddress(cache, notification.RemoteAccountKey);
		return !remoteAccountStateIter || state::AccountType::Remote_Unlinked == remoteAccountStateIter->AccountType
					   ? ValidationResult::Success
					   : Failure_AccountLink_Remote_Account_Ineligible;
	})
}}
