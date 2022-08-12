/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(BalanceCredit, model::BalanceCreditNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = cache.find(notification.Sender);
		auto& senderState = senderIter.get();

		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto amount = context.Resolvers.resolve(notification.Amount);
		if (NotifyMode::Commit == context.Mode)
			senderState.Balances.credit(mosaicId, amount, context.Height);
		else
			senderState.Balances.debit(mosaicId, amount, context.Height);
	});
}}
