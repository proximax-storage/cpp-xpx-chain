/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OperationCache.h"
#include "src/model/OperationIdentifyTransaction.h"

namespace catapult { namespace observers {

	using Notification = model::AggregateTransactionHashNotification<1>;

	DEFINE_OBSERVER(AggregateTransactionHash, Notification, [](const auto& notification, auto& context) {
		if (!notification.TransactionsCount || model::Entity_Type_OperationIdentify != notification.TransactionsPtr->Type)
			return;

		const auto& operationToken = static_cast<const model::EmbeddedOperationIdentifyTransaction&>(*notification.TransactionsPtr).OperationToken;
		if (Hash256() == operationToken)
			return;

		auto& operationCache = context.Cache.template sub<cache::OperationCache>();
		auto operationCacheIter = operationCache.find(operationToken);
		auto& operationEntry = operationCacheIter.get();

		if (NotifyMode::Commit == context.Mode) {
			operationEntry.TransactionHashes.push_back(notification.AggregateHash);
		} else {
			const auto& lastHash = operationEntry.TransactionHashes.back();
			if (lastHash != notification.AggregateHash)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid aggregate hash (expected, actual)", lastHash, notification.AggregateHash);
			operationEntry.TransactionHashes.pop_back();
		}
	});
}}
