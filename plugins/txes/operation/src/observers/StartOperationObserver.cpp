/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OperationCache.h"
#include "src/model/OperationReceiptType.h"

namespace catapult { namespace observers {

	using Notification = model::StartOperationNotification<1>;

	DEFINE_OBSERVER(StartOperation, Notification, [](const auto& notification, ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::OperationCache>();
		if (NotifyMode::Commit == context.Mode) {
			state::OperationEntry operationEntry(notification.OperationToken);
			operationEntry.Account = notification.Signer;
			operationEntry.Height = context.Height + Height(notification.Duration.unwrap());
			auto pMosaic = notification.MosaicsPtr;
			for (auto i = 0u; i < notification.MosaicCount; ++i, ++pMosaic) {
				auto mosaicId = context.Resolvers.resolve(pMosaic->MosaicId);
				operationEntry.Mosaics.emplace(mosaicId, pMosaic->Amount);

				auto receiptType = model::Receipt_Type_Operation_Started;
				model::BalanceChangeReceipt receipt(receiptType, notification.Signer, mosaicId, pMosaic->Amount);
				context.StatementBuilder().addTransactionReceipt(receipt);
			}
			auto pExecutor = notification.ExecutorsPtr;
			for (auto i = 0u; i < notification.ExecutorCount; ++i, ++pExecutor) {
				operationEntry.Executors.insert(*pExecutor);
			}
			cache.insert(operationEntry);
		} else {
			cache.remove(notification.OperationToken);
		}
	});
}}
