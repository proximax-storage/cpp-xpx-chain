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
#include "src/model/MosaicIdGenerator.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"
#include "catapult/cache_core/AccountStateCache.h"
namespace catapult { namespace validators {

	using Notification = model::MosaicNonceNotification<1>;

	DEFINE_STATELESS_VALIDATOR(MosaicId, [](const auto& notification) {
		if (MosaicId() == notification.MosaicId)
			return Failure_Mosaic_Invalid_Id;

		return notification.MosaicId == model::GenerateMosaicId(notification.Signer, notification.MosaicNonce)
				? ValidationResult::Success
				: Failure_Mosaic_Id_Mismatch;
	})

	DECLARE_STATEFUL_VALIDATOR(MosaicIdV2, model::MosaicNonceNotification<2>)() {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(MosaicIdV2, model::MosaicNonceNotification<2>, [](const auto& notification, const ValidatorContext& context) {
			if (MosaicId() == notification.MosaicId)
				return Failure_Mosaic_Invalid_Id;

			auto& accountStateCache = context.Cache.template sub<cache::AccountStateCache>();
			auto isFound = cache::FindActiveAccountKeyMatchBackwards(accountStateCache, notification.Signer, [nonce = notification.MosaicNonce, mosaicId = notification.MosaicId](Key relatedKey) {
				if(model::GenerateMosaicId(relatedKey, nonce) == mosaicId)
					return true;
				return false;
			});
			return isFound
						   ? ValidationResult::Success
						   : Failure_Mosaic_Id_Mismatch;
		});
	}
}}
