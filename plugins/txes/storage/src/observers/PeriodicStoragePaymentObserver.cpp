/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(PeriodicStoragePayment, Notification)() {
		return MAKE_OBSERVER(PeriodicStoragePayment, Notification, ([](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			CATAPULT_LOG( error ) << "Verification Observer";

			if (context.Height < Height(2))
				return;

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto queueIter = queueCache.find(state::DrivePaymentQueueKey);

			if (!queueIter.tryGet()) {
				return;
			}

			auto& queueEntry = queueIter.get();

			if (queueEntry.getFirst() == Key()) {
				// Empty Queue of Drives
				return;
			}

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto paymentInterval = pluginConfig.StorageBillingPeriod.seconds();

			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();

			auto lastDriveIter = driveCache.find(queueEntry.getLast());

			for (int i = 0; i < driveCache.size(); i++) {
				auto driveIter = driveCache.find(queueEntry.getFirst());
				auto& driveEntry = driveIter.get();

				auto timeSinceLastPayment = (notification.Timestamp - driveEntry.getLastPayment()).unwrap() / 1000;
				CATAPULT_LOG( error ) << "time " << timeSinceLastPayment << " " << paymentInterval;
				if (timeSinceLastPayment < paymentInterval) {
					break;
				}

				// Pop element from the Queue
				queueEntry.setFirst(driveEntry.getStoragePaymentsQueueNext());
				CATAPULT_LOG( error ) << "set first " << queueEntry.getFirst();
				if (driveEntry.getStoragePaymentsQueueNext() != Key())
				{
					// Previous of the first MUST be "null"
					driveCache.find(queueEntry.getFirst()).get().setStoragePaymentsQueuePrevious(Key());
				}
				else {
					// There is only one element in the Queue. So after removal there are no elements at all
					queueEntry.setFirst(Key());
					queueEntry.setLast(Key());
				}

				const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
				const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
				const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;

				auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
				auto driveStateIter = accountStateCache.find(driveEntry.key());
				auto& driveState = driveStateIter.get();

				for (auto& [replicatorKey, info]: driveEntry.confirmedStorageInfos()) {
					auto replicatorIter = accountStateCache.find(replicatorKey);
					auto& replicatorState = replicatorIter.get();

					if (info.m_confirmedStorageSince) {
						info.m_timeInConfirmedStorage = info.m_timeInConfirmedStorage
								+ notification.Timestamp - *info.m_confirmedStorageSince;
						info.m_confirmedStorageSince = notification.Timestamp;
					}
					BigUint driveSize = driveEntry.size();
					auto payment = Amount(((driveSize * info.m_timeInConfirmedStorage.unwrap()) / timeSinceLastPayment).template convert_to<uint64_t>());
					driveState.Balances.debit(storageMosaicId, payment, context.Height);
					replicatorState.Balances.credit(currencyMosaicId, payment, context.Height);
				}

				if (driveState.Balances.get(storageMosaicId).unwrap() >= driveEntry.size() * driveEntry.replicatorCount()) {

					// Drive Continues To Work
					CATAPULT_LOG( error ) << "drive continues " << driveEntry.key()
							<< " " << driveState.Balances.get(storageMosaicId).unwrap()
							<< " " << driveEntry.size()
							<< " " << driveEntry.replicatorCount();

					driveEntry.setLastPayment(notification.Timestamp);

					queueEntry.setLast(driveEntry.key());
					driveEntry.setStoragePaymentsQueueNext(Key());

					auto& lastDriveEntry = lastDriveIter.get();

					CATAPULT_LOG( error ) << "last " <<lastDriveEntry.key();

					if (queueEntry.getFirst() != Key()) {
						// The first and the last element are different
						lastDriveEntry.setStoragePaymentsQueueNext(driveEntry.key());
						driveEntry.setStoragePaymentsQueuePrevious(lastDriveEntry.key());
					}
					else {
						// There is only one element in the queue. Its next and previous must be null
						queueEntry.setFirst(driveEntry.key());
						lastDriveEntry.setStoragePaymentsQueueNext(Key());
						driveEntry.setStoragePaymentsQueuePrevious(Key());
					}

					lastDriveIter = driveIter;
				}
				else {
					// Drive is Closed
					CATAPULT_LOG( error ) << "drive is closed";
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

					auto driveOwnerIter = accountStateCache.find(driveEntry.owner());
					auto& driveOwnerState = driveOwnerIter.get();
					driveOwnerState.Balances.credit(currencyMosaicId, refundAmount, context.Height);

					// Removing the drive from caches
					auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
					for (const auto& replicatorKey : driveEntry.replicators())
						replicatorCache.find(replicatorKey).get().drives().erase(driveEntry.key());

					driveCache.remove(driveEntry.key());

					// TODO Replicators assignment
				}
			}
        }))
	};
}}