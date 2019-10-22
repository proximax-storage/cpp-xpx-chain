/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/model/EntityType.h"
#include "src/model/ServiceEntityType.h"
#include "catapult/model/TransactionPlugin.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateEmbeddedTransactionNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DrivePermittedOperation, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (!driveCache.contains(notification.Signer))
			return ValidationResult::Success;

		std::vector<model::EntityType> allowedTransactions({ model::Entity_Type_DeleteReward, model::Entity_Type_EndDrive, model::Entity_Type_Verification });

		return std::find(allowedTransactions.begin(), allowedTransactions.end(), notification.Transaction.Type) != allowedTransactions.end() ?
			ValidationResult::Success : Failure_Service_Operation_Is_Not_Permitted;
	});
}}
