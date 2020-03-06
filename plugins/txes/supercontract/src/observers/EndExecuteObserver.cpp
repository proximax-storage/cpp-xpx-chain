/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/model/EndExecuteTransaction.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace observers {

	using Notification = model::AggregateCosignaturesNotification<2>;

	DEFINE_OBSERVER(EndExecute, Notification, ([](const auto& notification, const ObserverContext& context) {
		const auto* pTransaction = notification.TransactionsPtr;
		if (!pTransaction || model::Entity_Type_EndExecute != pTransaction->Type)
			return;

		const auto& endExecuteTransaction = static_cast<const model::EmbeddedEndExecuteTransaction&>(*pTransaction);

		auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		auto superContractCacheIter = superContractCache.find(endExecuteTransaction.Signer);
		auto& superContractEntry = superContractCacheIter.get();
		if (NotifyMode::Commit == context.Mode) {
			superContractEntry.decrementExecutionCount();
		} else {
			superContractEntry.incrementExecutionCount();
		}

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		std::vector<cache::AccountStateCacheDeltaMixins::MutableAccessorKey::iterator> iterators;
		iterators.reserve(notification.CosignaturesCount);
		std::map<Key, state::AccountState&> accounts;

		auto pMosaic = endExecuteTransaction.MosaicsPtr();
		for (auto i = 0u; i < endExecuteTransaction.MosaicCount; ++i, ++pMosaic) {
			auto mosaicId = context.Resolvers.resolve(pMosaic->MosaicId);
			auto totalAmount = pMosaic->Amount.unwrap();
			auto payment = Amount(totalAmount / notification.CosignaturesCount + 1);
			auto remainder = totalAmount % notification.CosignaturesCount;

			auto pCosignature = notification.CosignaturesPtr;
			for (auto k = 0u; k < notification.CosignaturesCount; ++k, ++pCosignature) {
				auto accountIter = accounts.find(pCosignature->Signer);
				if (accounts.end() == accountIter) {
					auto accountCacheIter = accountStateCache.find(pCosignature->Signer);
					iterators.push_back(std::move(accountCacheIter));
					accounts.emplace(pCosignature->Signer, iterators.back().get());
					accountIter = accounts.find(pCosignature->Signer);
				}
				auto& accountState = accountIter->second;

				if (k == remainder) {
					payment = payment - Amount(1);
				}

				if (NotifyMode::Commit == context.Mode) {
					accountState.Balances.credit(mosaicId, payment, context.Height);
				} else {
					accountState.Balances.debit(mosaicId, payment, context.Height);
				}
			}
		}
	}));
}}
