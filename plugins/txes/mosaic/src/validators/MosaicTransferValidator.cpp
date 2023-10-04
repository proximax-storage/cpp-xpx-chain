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
#include "ActiveMosaicView.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::BalanceTransferNotification<1>;

	namespace {
		bool IsMosaicOwnerParticipant(
				const cache::ReadOnlyCatapultCache& cache,
				const Key& owner,
				const Notification& notification,
				const model::ResolverContext& resolvers) {
			const auto& accountStateCache = cache.sub<cache::AccountStateCache>();
			/// IF owner is the same as the sender.
			if(cache::FindActiveAccountKeyMatchBackwards(accountStateCache, notification.Sender, [mosaicOwner = owner](Key relatedKey) {
				if(mosaicOwner == relatedKey)
					return true;
				return false;
			}))
				return true;

			// the owner must exist if the mosaic lookup succeeded

			auto ownerAccountStateIter = accountStateCache.find(owner);
			return cache::FindActiveAccountAddressMatchBackwards(accountStateCache, resolvers.resolve(notification.Recipient), [address = ownerAccountStateIter.get().Address](Address relatedAddress) {
				if(address == relatedAddress)
					return true;
				return false;
			});
		}
	}

	DECLARE_STATEFUL_VALIDATOR(MosaicTransfer, Notification)(UnresolvedMosaicId currencyMosaicId) {
		return MAKE_STATEFUL_VALIDATOR(MosaicTransfer, [currencyMosaicId](const auto& notification, const auto& context) {
			// 0. allow currency mosaic id
			if (currencyMosaicId == notification.MosaicId)
				return ValidationResult::Success;

			// 1. check that the mosaic exists
			ActiveMosaicView::FindIterator mosaicIter;
			ActiveMosaicView activeMosaicView(context.Cache);
			auto result = activeMosaicView.tryGet(context.Resolvers.resolve(notification.MosaicId), context.Height, mosaicIter);
			if (!IsValidationResultSuccess(result))
				return result;

			// 2. if it's transferable there's nothing else to check
			const auto& entry = mosaicIter.get();
			if (entry.definition().properties().is(model::MosaicFlags::Transferable))
				return ValidationResult::Success;

			const auto& accountStateCache = context.Cache.template sub<cache::AccountStateCache>();
			// 3. if it's NOT transferable then owner must be either sender or recipient


			if (!IsMosaicOwnerParticipant(context.Cache, entry.definition().owner(), notification, context.Resolvers))
				return Failure_Mosaic_Non_Transferable;

			return ValidationResult::Success;
		});
	}
}}
