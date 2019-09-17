/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DriveProlongation, model::DriveProlongationNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto& driveEntry = driveCache.find(notification.Drive).get();
		auto& customerDepositMap = driveEntry.customers()[notification.Customer];
		auto& customerDeposit = customerDepositMap[Hash256()];
		if (NotifyMode::Commit == context.Mode) {
			driveEntry.setDuration(BlockDuration{driveEntry.duration().unwrap() + notification.Duration.unwrap()});
			customerDeposit.Amount = Amount{customerDeposit.Amount.unwrap() + notification.Deposit.Amount.unwrap()};
		} else {
			driveEntry.setDuration(BlockDuration{driveEntry.duration().unwrap() - notification.Duration.unwrap()});
			customerDeposit.Amount = Amount{customerDeposit.Amount.unwrap() - notification.Deposit.Amount.unwrap()};
			if (Amount{0} == customerDeposit.Amount)
				customerDepositMap.erase(Hash256());
		}
	});
}}
