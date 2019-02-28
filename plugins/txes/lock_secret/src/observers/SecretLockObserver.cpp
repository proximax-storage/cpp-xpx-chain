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

#include "Observers.h"
#include "src/cache/SecretLockInfoCache.h"
#include "src/model/SecretLockReceiptType.h"
#include "src/state/SecretLockInfo.h"

namespace catapult { namespace observers {

	using Notification = model::SecretLockNotification;

	namespace {
		auto CreateLockInfo(
				const Key& account,
				MosaicId mosaicId,
				Height endHeight,
				const Notification& notification,
				const model::ResolverContext& resolvers) {
			return state::SecretLockInfo(
					account,
					mosaicId,
					notification.Mosaic.Amount,
					endHeight,
					notification.HashAlgorithm,
					notification.Secret,
					resolvers.resolve(notification.Recipient));
		}
	}

	DEFINE_OBSERVER(SecretLock, Notification, [](const auto& notification, ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::SecretLockInfoCache>();
		if (NotifyMode::Commit == context.Mode) {
			auto endHeight = context.Height + Height(notification.Duration.unwrap());
			auto mosaicId = context.Resolvers.resolve(notification.Mosaic.MosaicId);
			cache.insert(CreateLockInfo(notification.Signer, mosaicId, endHeight, notification, context.Resolvers));

			auto receiptType = model::Receipt_Type_LockSecret_Created;
			model::BalanceChangeReceipt receipt(receiptType, notification.Signer, mosaicId, notification.Mosaic.Amount);
			context.StatementBuilder().addReceipt(receipt);
		} else {
			cache.remove(notification.Secret);
		}
	});
}}
