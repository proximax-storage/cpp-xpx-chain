/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::ReleasedTransactionsNotification<1>;

	DEFINE_OBSERVER(ReleasedTransactions, Notification , [](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReleasedTransactions)");

		auto contractKey = *notification.SubTransactionsSigners.begin();
		auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(contractKey);
		auto& contractEntry = contractIt.get();

		auto& releasedTransactions = contractEntry.releasedTransactions();
		auto it = releasedTransactions.find(notification.PayloadHash);
		releasedTransactions.erase(it);
	})
}
