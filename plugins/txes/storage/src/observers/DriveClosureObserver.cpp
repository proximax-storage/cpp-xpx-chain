/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>, ([](const auto& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
	  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
	  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
	  	auto driveStateIter = accountStateCache.find(notification.DriveKey);
	  	auto& driveState = driveStateIter.get();
	  	auto driveOwnerIter = accountStateCache.find(driveEntry.owner());
	  	auto& driveOwnerState = driveOwnerIter.get();

	  	// Making payments to replicators, if there is a pending data modification
	  	auto& activeDataModifications = driveEntry.activeDataModifications();
	  	if (!activeDataModifications.empty()) {
			const auto& modificationSize = activeDataModifications.front().ExpectedUploadSizeMegabytes;
		  	const auto& replicators = driveEntry.replicators();
		  	const auto totalReplicatorAmount = Amount(
				  	modificationSize +	// Download work
				  	modificationSize * (replicators.size() - 1) / replicators.size());	// Upload work
		  	for (const auto& replicatorKey : replicators) {
			  	auto replicatorIter = accountStateCache.find(replicatorKey);
			  	auto& replicatorState = replicatorIter.get();
			  	driveState.Balances.debit(streamingMosaicId, totalReplicatorAmount, context.Height);
			  	replicatorState.Balances.credit(currencyMosaicId, totalReplicatorAmount, context.Height);
		  	}
	  	}

		// Returning the rest to the drive owner
		const auto refundAmount = driveState.Balances.get(streamingMosaicId);
	  	driveState.Balances.debit(streamingMosaicId, refundAmount, context.Height);
	  	driveOwnerState.Balances.credit(currencyMosaicId, refundAmount, context.Height);

		// Removing the drive from caches
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicatorKey : driveEntry.replicators())
			replicatorCache.find(replicatorKey).get().drives().erase(notification.DriveKey);

		driveCache.remove(notification.DriveKey);
	}));
}}
