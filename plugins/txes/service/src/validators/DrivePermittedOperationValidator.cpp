/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/model/EntityType.h"
#include "src/model/ServiceEntityType.h"
#include "plugins/txes/exchange/src/model/ExchangeTransaction.h"
#include "catapult/model/TransactionPlugin.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateEmbeddedTransactionNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DrivePermittedOperation, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (!driveCache.contains(notification.Transaction.Signer))
			return ValidationResult::Success;

		static std::unordered_set<model::EntityType> allowedTransactions({
		   model::Entity_Type_DeleteReward,
		   model::Entity_Type_EndDrive,
		   model::Entity_Type_Start_Drive_Verification,
		   model::Entity_Type_End_Drive_Verification,
		   model::Entity_Type_Exchange,
		});

		return allowedTransactions.count(notification.Transaction.Type) ?
			ValidationResult::Success : Failure_Service_Operation_Is_Not_Permitted;
	});
}}
