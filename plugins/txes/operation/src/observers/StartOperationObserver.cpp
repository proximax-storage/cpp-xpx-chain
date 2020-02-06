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
#include "src/cache/OperationCache.h"
#include "src/model/OperationReceiptType.h"
#include "src/state/OperationEntry.h"

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
