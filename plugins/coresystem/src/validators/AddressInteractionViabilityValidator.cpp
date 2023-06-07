/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "catapult/model/Address.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace validators {

	using Notification = model::AddressInteractionNotification<1>;

	namespace {
		ValidationResult Validate(const Notification& notification, const ValidatorContext& context) {
			const auto& cache = context.Cache.sub<cache::AccountStateCache>();
			for (const auto& address : notification.ParticipantsByAddress) {
				auto participant = context.Resolvers.resolve(address);
				const auto& account = cache.find(participant).tryGet();
				if(account && account->IsLocked()) {
					return Failure_Core_Participant_Is_Locked;
				}
			}
			return ValidationResult::Success;
		}
	}

	DEFINE_STATEFUL_VALIDATOR(AddressInteractionViability, Validate)
}}
