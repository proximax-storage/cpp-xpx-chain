/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>
#include "Queue.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(PeriodicStoragePayment, Notification)(const std::shared_ptr<DriveQueue>& pDriveQueue, const LiquidityProviderExchangeObserver& liquidityProvider) {
		return MAKE_OBSERVER(PeriodicStoragePayment, Notification, ([pDriveQueue, &liquidityProvider](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			if (context.Height < Height(2))
				return;

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();

			QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);

			if (queueAdapter.isEmpty()) {
				return;
			}

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto paymentInterval = pluginConfig.StorageBillingPeriod.seconds();

			// Creating unique eventHash for the observer
			Hash256 eventHash;
			crypto::Sha3_256_Builder sha3;
			const std::string salt = "Storage";
			sha3.update({notification.Hash,
						 utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()),
						 context.Config.Immutable.GenerationHash});
			sha3.final(eventHash);

			for (int i = 0; i < driveCache.size(); i++) {
				auto driveIter = driveCache.find(queueAdapter.front());
				auto& driveEntry = driveIter.get();

				auto timeSinceLastPayment = (notification.Timestamp - driveEntry.getLastPayment()).unwrap() / 1000;
				if (timeSinceLastPayment < paymentInterval) {
					break;
				}

				queueAdapter.popFront();

				const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
				const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
				const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;

				const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
				auto driveStateIter = accountStateCache.find(driveEntry.key());
				const auto& driveState = driveStateIter.get();

				for (auto& [replicatorKey, info]: driveEntry.confirmedStorageInfos()) {
					if (info.m_confirmedStorageSince) {
						info.m_timeInConfirmedStorage = info.m_timeInConfirmedStorage
								+ notification.Timestamp - *info.m_confirmedStorageSince;
						info.m_confirmedStorageSince = notification.Timestamp;
					}
					BigUint driveSize = driveEntry.size();
					auto payment = Amount(((driveSize * info.m_timeInConfirmedStorage.unwrap()) / timeSinceLastPayment).template convert_to<uint64_t>());
					liquidityProvider.debitMosaics(context, driveEntry.key(), replicatorKey, config::GetUnresolvedStorageMosaicId(context.Config.Immutable), payment);
				}

				if (driveState.Balances.get(storageMosaicId).unwrap() >= driveEntry.size() * driveEntry.replicatorCount()) {

					// Drive Continues To Work
					driveEntry.setLastPayment(notification.Timestamp);
					queueAdapter.pushBack(driveEntry.entryKey());
				}
				else {
					// Drive is Closed

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
									liquidityProvider.debitMosaics(context, driveEntry.key(), replicatorKey, config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), totalReplicatorAmount);
								}
					}

					// Returning the rest to the drive owner
					const auto refundStreamingAmount = driveState.Balances.get(streamingMosaicId);
					liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), refundStreamingAmount);

					const auto refundStorageAmount = driveState.Balances.get(storageMosaicId);
					liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStorageMosaicId(context.Config.Immutable), refundStorageAmount);

					// Simulate publishing of finish download for all download channels

					auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
					for (const auto& key: driveEntry.downloadShards()) {
						auto downloadIter = downloadCache.find(key);
						auto& downloadEntry = downloadIter.get();
						if (!downloadEntry.isCloseInitiated()) {
							downloadEntry.setFinishPublished(true);
							downloadEntry.downloadApprovalInitiationEvent() = eventHash;
						}
					}

					// Removing the drive from caches
					const auto replicators = driveEntry.replicators();
					auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
					for (const auto& replicatorKey : replicators)
						replicatorCache.find(replicatorKey).get().drives().erase(driveEntry.key());

					driveCache.remove(driveEntry.key());

					// Assigning drive's former replicators to queued drives
					std::seed_seq seed(eventHash.begin(), eventHash.end());
					std::mt19937 rng(seed);
					utils::AssignReplicatorsToQueuedDrives(replicators, pDriveQueue, context, rng);
				}
			}
        }))
	};
}}