/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OperationCache.h"
#include "src/model/OperationReceiptType.h"
#include "plugins/txes/lock_shared/src/observers/LockStatusAccountBalanceObserver.h"

namespace catapult { namespace observers {

	using Notification = model::EndOperationNotification<1>;

	namespace {
		struct OperationTraits {
		public:
			using CacheType = cache::OperationCache;
			using Notification = observers::Notification;
			static constexpr auto Receipt_Type = model::Receipt_Type_Operation_Ended;

			static auto NotificationToKey(const Notification& notification, const model::ResolverContext&) {
				return notification.OperationToken;
			}

			static auto DestinationAccount(const state::OperationEntry& entry) {
				return entry.Account;
			}
		};
	}

	DEFINE_OBSERVER(EndOperation, Notification, [](const auto& notification, auto& context) {
		auto& operationCache = context.Cache.template sub<cache::OperationCache>();
		auto operationCacheIter = operationCache.find(notification.OperationToken);
		auto& operationEntry = operationCacheIter.get();
		auto& mosaics = operationEntry.Mosaics;
		auto pMosaic = notification.MosaicsPtr;

		if (NotifyMode::Commit == context.Mode) {
			operationEntry.Result = notification.Result;
			for (auto i = 0u; i < notification.MosaicCount; ++i, ++pMosaic) {
				auto mosaicId = context.Resolvers.resolve(pMosaic->MosaicId);
				auto& amount = mosaics.at(mosaicId);
				amount = amount - pMosaic->Amount;
				if (Amount(0) == amount)
					mosaics.erase(mosaicId);
			}
			if (mosaics.empty())
				operationEntry.Status = state::LockStatus::Used;
		}

		LockStatusAccountBalanceObserver<OperationTraits>(notification, context);

		if (NotifyMode::Rollback == context.Mode) {
			operationEntry.Result = model::Operation_Result_None;
			operationEntry.Status = state::LockStatus::Unused;
			for (auto i = 0u; i < notification.MosaicCount; ++i, ++pMosaic) {
				auto mosaicId = context.Resolvers.resolve(pMosaic->MosaicId);
				auto& amount = mosaics[mosaicId];
				amount = amount + pMosaic->Amount;
			}
		}
	});
}}
