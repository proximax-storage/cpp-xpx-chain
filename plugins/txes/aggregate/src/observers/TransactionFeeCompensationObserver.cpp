/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include <src/model/AggregateEntityType.h>
#include <src/model/AggregateTransaction.h>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::TransactionFeeNotification<1>;

	DECLARE_OBSERVER(TransactionFeeCompensation, Notification)() {
		return MAKE_OBSERVER(BatchCalls, Notification, ([](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (TransactionFeeCompensation)");

			if (notification.TransactionEntity.Type != model::Entity_Type_Aggregate_Complete ||
			notification.TransactionEntity.Version != 4) {
				return;
			}

			const auto& transaction = static_cast<const model::AggregateTransaction&>(notification.TransactionEntity);

			const auto actualSigner = transaction.Transactions().cbegin()->Signer;

			auto& accountCache = context.Cache.template sub<cache::AccountStateCache>();
			auto actualSignerAccountIt = accountCache.find(actualSigner);
			auto& actualSignerAccountEntry = actualSignerAccountIt.get();

			auto feeMosaicId = context.Resolvers.resolve(notification.MosaicId);

			actualSignerAccountEntry.Balances.debit(feeMosaicId, notification.Fee);

			auto nominalSignerAccountIt = accountCache.find(transaction.Signer);
			auto& nominalSignerAccountEntry = nominalSignerAccountIt.get();

			nominalSignerAccountEntry.Balances.credit(feeMosaicId, notification.Fee);
		}))
	}
}
