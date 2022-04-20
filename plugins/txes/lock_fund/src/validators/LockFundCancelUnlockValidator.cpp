/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/cache/LockFundCache.h>
#include <catapult/cache_core/AccountStateCache.h>
#include "Validators.h"
#include "src/config/LockFundConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::LockFundCancelUnlockNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(LockFundCancelUnlock, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(LockFundCancelUnlock, ([](const Notification& notification, const ValidatorContext& context) {
            auto& lockFundCache = context.Cache.sub<cache::LockFundCache>();
			auto keyRecordIt = lockFundCache.find(notification.Sender);
			auto keyRecord = keyRecordIt.tryGet();
			if(!keyRecord)
				return Failure_LockFund_Request_Non_Existant;
			if(keyRecord->LockFundRecords.find(notification.TargetHeight) == keyRecord->LockFundRecords.end() || notification.TargetHeight <= context.Height)
				return Failure_LockFund_Request_Non_Existant;
			return ValidationResult::Success;
		}));
	}
}}
