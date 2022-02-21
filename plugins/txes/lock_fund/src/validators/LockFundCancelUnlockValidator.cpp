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
			if(keyRecord->LockFundRecords.find(notification.TargetHeight) == keyRecord->LockFundRecords.end())
				return Failure_LockFund_Request_Non_Existant;
			return ValidationResult::Success;
		}));
	}
}}
