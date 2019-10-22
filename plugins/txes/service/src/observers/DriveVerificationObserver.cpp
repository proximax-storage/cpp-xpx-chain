/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/model/NetworkConfiguration.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DriveVerification, model::DriveVerificationNotification<1>, [](const auto&, const ObserverContext&) {
//		auto& driveCache = context.Cache.sub<cache::DriveCache>();
//		auto& driveEntry = driveCache.find(notification.Drive).get();
//		auto& replicatorDepositMap = driveEntry.customers()[notification.Replicator];
//		auto& replicatorDeposit = replicatorDepositMap[Hash256()];
//		if (NotifyMode::Commit == context.Mode) {
//			replicatorDeposit.Amount = Amount{replicatorDeposit.Amount.unwrap() + notification.Deposit.Amount.unwrap()};
//		} else {
//			replicatorDeposit.Amount = Amount{replicatorDeposit.Amount.unwrap() - notification.Deposit.Amount.unwrap()};
//			if (Amount{0} == replicatorDeposit.Amount)
//				replicatorDepositMap.erase(Hash256());
//		}
	});
}}
