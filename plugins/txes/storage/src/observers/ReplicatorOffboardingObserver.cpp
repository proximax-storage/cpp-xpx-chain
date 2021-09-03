/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include <algorithm>

namespace catapult { namespace observers { 

	DEFINE_OBSERVER(ReplicatorOffboarding, model::ReplicatorOffboardingNotification<1>, [](const model::ReplicatorOffboardingNotification<1>& notification, ObserverContext& context) {
		//Replicator entry
	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(notification.PublicKey);
		auto& replicatorEntry = replicatorIter.get();

		//Replicator account state
		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto accountIter = cache.find(notification.PublicKey);
		auto& replicatorState = accountIter.get();

		//Remove replicator public key
		replicatorCache.remove(notification.PublicKey);

		//Storage deposit return
		//Mosaic id
		const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;
		const auto streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		
		//Remaining capacity
		auto deposit = replicatorEntry.capacity().unwrap();

		//To get Total drive size served and Total driveSize
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		uint64_t driveSize = 0;
		for(const auto& iter : replicatorEntry.drives()){
			auto driveIter = driveCache.find(iter.first);
			const auto& drive = driveIter.get();		
			deposit += drive.size();
			driveSize += drive.size();
		}

		//Streaming deposit slashing
		//To get UsedDriveSize of last approved by the Replicator modification
		uint64_t usedDriveSizeOnReplicator = 0;
        for(const auto& i : replicatorEntry.drives()){
            auto driveIter = driveCache.find(i.first);
            const auto& drive = driveIter.get();   
			for(const auto& j : drive.confirmedUsedSizes()){
				usedDriveSizeOnReplicator += j.second;
			} 
        }

        //To get UsedDriveSize of last approved modification on the Drive 
        uint64_t usedDriveSizeOnDrive = 0;
        for(const auto& i : replicatorEntry.drives()){
            auto driveIter = driveCache.find(i.first);
            const auto& drive = driveIter.get();        
            usedDriveSizeOnDrive += drive.usedSize();
        }
		
		//Streaming deposit slashing = 2min(usedDriveSizeOnReplicator, usedDriveSizeOnDrive)
		auto streamingDepositSlashing = 2 * std::min(usedDriveSizeOnReplicator, usedDriveSizeOnDrive);
		
		//Streaming deposit return = streaming deposit - streaming deposit slashing
		auto streamingDeposit = (driveSize * 2) - streamingDepositSlashing;

		if (NotifyMode::Commit == context.Mode)
			replicatorState.Balances.credit(storageMosaicId, Amount(deposit + streamingDeposit), context.Height);
		else
			replicatorState.Balances.debit(storageMosaicId, Amount(deposit + streamingDeposit), context.Height);
	});
}}
