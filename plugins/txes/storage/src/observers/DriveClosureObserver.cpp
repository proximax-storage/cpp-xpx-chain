/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using BigUint = boost::multiprecision::uint128_t;

    DEFINE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>, ([](const auto& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		auto& queueCache = context.Cache.sub<cache::QueueCache>();
		auto queueIter = queueCache.find(state::DrivePaymentQueueKey);
		auto& queueEntry = queueIter.get();

	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
	  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
	  	const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
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

	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
	  	auto paymentInterval = pluginConfig.StorageBillingPeriod.seconds();

	  	auto timeSinceLastPayment = (context.Timestamp - driveEntry.getLastPayment()).unwrap() / 1000;
	  	for (auto& [replicatorKey, info]: driveEntry.confirmedStorageInfos()) {
	  		auto replicatorIter = accountStateCache.find(replicatorKey);
	  		auto& replicatorState = replicatorIter.get();

	  		if (info.m_confirmedStorageSince) {
	  			info.m_timeInConfirmedStorage = info.m_timeInConfirmedStorage
	  					+ context.Timestamp - *info.m_confirmedStorageSince;
	  			info.m_confirmedStorageSince = context.Timestamp;
	  		}
	  		BigUint driveSize = driveEntry.size();
	  		auto payment = Amount(((driveSize * info.m_timeInConfirmedStorage.unwrap()) / paymentInterval).template convert_to<uint64_t>());
	  		driveState.Balances.debit(storageMosaicId, payment, context.Height);
	  		replicatorState.Balances.credit(currencyMosaicId, payment, context.Height);
		}

	  	// The Drive is Removed, so we should make removal in linked list
	  	if (driveEntry.getStoragePaymentsQueuePrevious() != Key()) {
	  		auto previousDriveIter = driveCache.find(driveEntry.getStoragePaymentsQueuePrevious());
			auto& previousDriveEntry = previousDriveIter.get();
			previousDriveEntry.setStoragePaymentsQueueNext(driveEntry.getStoragePaymentsQueueNext());
		}
		else {
			// Previous link is "null" so the Drive is first in the queue
			queueEntry.setFirst(driveEntry.getStoragePaymentsQueueNext());
			if (driveEntry.getStoragePaymentsQueueNext() != Key()) {
				auto nextDriveIter = driveCache.find(driveEntry.getStoragePaymentsQueueNext());
				auto& nextDriveEntry = nextDriveIter.get();
				nextDriveEntry.setStoragePaymentsQueuePrevious(Key());
			}
		}

	  	if (driveEntry.getStoragePaymentsQueueNext() != Key()) {
	  		auto nextDriveIter = driveCache.find(driveEntry.getStoragePaymentsQueueNext());
	  		auto& nextDriveEntry = nextDriveIter.get();
	  		nextDriveEntry.setStoragePaymentsQueuePrevious(driveEntry.getStoragePaymentsQueuePrevious());
	  	}
		else {
			queueEntry.setLast(driveEntry.getStoragePaymentsQueuePrevious());
			if (driveEntry.getStoragePaymentsQueuePrevious() != Key()) {
				auto previousDriveIter = driveCache.find(driveEntry.getStoragePaymentsQueuePrevious());
				auto& previousDriveEntry = previousDriveIter.get();
				previousDriveEntry.setStoragePaymentsQueueNext(Key());
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
