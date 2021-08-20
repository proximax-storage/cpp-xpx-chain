/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers { 

	DEFINE_OBSERVER(ReplicatorOffboarding, model::ReplicatorOffboardingNotification<1>, [](const model::ReplicatorOffboardingNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOffboarding)");

	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		replicatorCache.remove(notification.PublicKey);
		
		//Replicator entry
		auto replicatorIter = replicatorCache.find(notification.PublicKey);
		auto& replicatorEntry = replicatorIter.get();

		//Replicator state
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto accountIter = cache.find(notification.PublicKey);
		auto& replicatorState = accountIter.get();

		//Mosaic id
		const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;
		const auto streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		
		//Remaining capacity
		auto deposit = replicatorEntry.capacity().unwrap();

		//Total drive size served
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		uint64_t driveSize = 0;
		for(const auto& iter : replicatorEntry.drives()){
			auto driveIter = driveCache.find(iter.first);
			const auto& drive = driveIter.get();		
			deposit += drive.size();
		}

		if (NotifyMode::Commit == context.Mode)
			replicatorState.Balances.credit(storageMosaicId, Amount(deposit), context.Height);
		else
			replicatorState.Balances.debit(storageMosaicId, Amount(deposit), context.Height);

		//UsedDriveSize of last approved by the Replicator modification

        // UsedDriveSize of last approved modification on the Drive 
        uint64_t usedDriveSizeOnDrive = 0;
        for(const auto& iter : replicatorEntry.drives()){
            auto driveIter = driveCache.find(iter.first);
            const auto& drive = driveIter.get();        
            usedDriveSizeOnDrive += drive.usedSize();
        }
	});
}}
