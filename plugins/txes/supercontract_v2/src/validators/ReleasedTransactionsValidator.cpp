/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ReleasedTransactionsNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReleasedTransactions, [](const Notification& notification, const ValidatorContext& context) {

		if (notification.SubTransactionsSigners.size() != 1) {
			return Failure_SuperContract_Invalid_Number_Of_Subtransactions_Cosigners;
		}

		auto contractKey = *notification.SubTransactionsSigners.begin();

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(contractKey);
		const auto* pContractEntry = contractIt.tryGet();

		if (!pContractEntry) {
			return Failure_SuperContract_Contract_Does_Not_Exist;
		}

		if (pContractEntry->releasedTransactions().find(notification.PayloadHash) ==
			pContractEntry->releasedTransactions().end()) {
			return Failure_SuperContract_Invalid_Released_Transactions_Hash;
		}

		return ValidationResult::Success;
	})

}}
