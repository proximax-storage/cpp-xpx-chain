/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Observers.h"
#include "src/cache/OperationCache.h"

namespace catapult { namespace observers {

	template<typename TTransaction>
	void AggregateTransactionHashObserver(const model::AggregateTransactionHashNotification<1>& notification, ObserverContext& context) {
		const model::EmbeddedTransaction* pTransaction = nullptr;
		if (notification.TransactionsCount) {
			const auto* pSubTransaction = notification.TransactionsPtr;
			if (TTransaction::Entity_Type == pSubTransaction->Type) {
				pTransaction = pSubTransaction;
			} else {
				for (auto i = 1u; i < notification.TransactionsCount; ++i) {
					pSubTransaction = model::AdvanceNext(pSubTransaction);
					if (TTransaction::Entity_Type == pSubTransaction->Type) {
						pTransaction = pSubTransaction;
						break;
					}
				}
			}
		}

		if (!pTransaction)
			return;

		const auto& operationToken = static_cast<const TTransaction&>(*pTransaction).OperationToken;
		auto& operationCache = context.Cache.sub<cache::OperationCache>();
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
	}
}}
