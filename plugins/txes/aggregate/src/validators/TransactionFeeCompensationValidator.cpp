/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include <src/model/AggregateEntityType.h>
#include <src/model/AggregateTransaction.h>
#include <catapult/cache_core/AccountStateCache.h>

namespace catapult { namespace validators {

	using Notification = model::TransactionFeeNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(TransactionFeeCompensation, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(TransactionFeeCompensation, ([](const auto& notification, const auto& context) {
		   if (notification.TransactionEntity.Type != model::Entity_Type_Aggregate_Complete ||
			   notification.TransactionEntity.Version != 4) {
			   return ValidationResult::Success;
		   }

		   const auto& transaction = static_cast<const model::AggregateTransaction&>(notification.TransactionEntity);

		   if (std::distance(transaction.Transactions().cbegin(), transaction.Transactions().cend()) == 0) {
			   return Failure_Aggregate_No_Transactions;
		   }

		   const auto actualSigner = transaction.Transactions().cbegin()->Signer;

		   const auto& accountCache = context.Cache.template sub<cache::AccountStateCache>();
		   auto actualSignerAccountIt = accountCache.find(actualSigner);

		   const auto* actualSignerAccountEntry = actualSignerAccountIt.tryGet();

		   if (!actualSignerAccountEntry) {
			   return Failure_Aggregate_Invalid_Cosigner;
		   }

		   auto feeMosaicId = context.Resolvers.resolve(notification.MosaicId);

		   if (actualSignerAccountEntry->Balances.get(feeMosaicId) < notification.Fee) {
			   return Failure_Aggregate_Insufficient_Fee_Compensation_Balance;
		   };

		   return ValidationResult::Success;
		}))
	}
}}
