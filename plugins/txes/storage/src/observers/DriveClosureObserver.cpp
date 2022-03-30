/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "Queue.h"
#include "src/utils/StorageUtils.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::DriveClosureNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;
	using BigUint = boost::multiprecision::uint128_t;

	DECLARE_OBSERVER(DriveClosure, Notification)(const std::shared_ptr<DriveQueue>& pDriveQueue, const LiquidityProviderExchangeObserver& liquidityProvider) {
		return MAKE_OBSERVER(
				DriveClosure,
				Notification,
				([pDriveQueue, &liquidityProvider](const Notification& notification, ObserverContext& context) {
					if (NotifyMode::Rollback == context.Mode)
						CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

					auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
					auto driveIter = driveCache.find(notification.DriveKey);
					auto& driveEntry = driveIter.get();

					const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
					const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
					const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
					const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
					auto driveStateIter = accountStateCache.find(notification.DriveKey);
					const auto& driveState = driveStateIter.get();

					// Making payments to replicators, if there is a pending data modification
					auto& activeDataModifications = driveEntry.activeDataModifications();
					if (!activeDataModifications.empty()) {
						const auto& modificationSize = activeDataModifications.front().ExpectedUploadSizeMegabytes;
						const auto& replicators = driveEntry.replicators();
						const auto totalReplicatorAmount =
								Amount(modificationSize + // Download work
									   modificationSize * (replicators.size() - 1) / replicators.size()); // Upload work
						for (const auto& replicatorKey : replicators) {
							liquidityProvider.debitMosaics(context, driveEntry.key(), replicatorKey, config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), totalReplicatorAmount);
						}
					}

					const auto& pluginConfig =
							context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
					auto paymentInterval = pluginConfig.StorageBillingPeriod.seconds();

					auto timeSinceLastPayment = (context.Timestamp - driveEntry.getLastPayment()).unwrap() / 1000;
					for (auto& [replicatorKey, info] : driveEntry.confirmedStorageInfos()) {
						auto replicatorIter = accountStateCache.find(replicatorKey);
						auto& replicatorState = replicatorIter.get();

						if (info.m_confirmedStorageSince) {
							info.m_timeInConfirmedStorage =
									info.m_timeInConfirmedStorage + context.Timestamp - *info.m_confirmedStorageSince;
							info.m_confirmedStorageSince = context.Timestamp;
						}
						BigUint driveSize = driveEntry.size();
						auto payment = Amount(((driveSize * info.m_timeInConfirmedStorage.unwrap()) / paymentInterval)
													  .template convert_to<uint64_t>());
						liquidityProvider.debitMosaics(context, driveEntry.key(), replicatorKey, config::GetUnresolvedStorageMosaicId(context.Config.Immutable), payment);
					}

					// The Drive is Removed, so we should make removal in linked list
					auto& queueCache = context.Cache.sub<cache::QueueCache>();
					QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);
					queueAdapter.remove(driveEntry.entryKey());

					// Returning the rest to the drive owner
					const auto refundStreamingAmount = driveState.Balances.get(streamingMosaicId);
					liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), refundStreamingAmount);

					const auto refundStorageAmount = driveState.Balances.get(storageMosaicId);
					liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStorageMosaicId(context.Config.Immutable), refundStorageAmount);

					// Removing the drive from queue, if necessary
					const auto replicators = driveEntry.replicators();
					if (replicators.size() < driveEntry.replicatorCount()) {
						DriveQueue originalQueue = *pDriveQueue;
						DriveQueue newQueue;
						while (!originalQueue.empty()) {
							const auto drivePriorityPair = originalQueue.top();
							const auto& driveKey = drivePriorityPair.first;
							originalQueue.pop();

							if (driveKey != notification.DriveKey)
								newQueue.push(drivePriorityPair);
						}
						*pDriveQueue = std::move(newQueue);
					}

					// Simulate publishing of finish download for all download channels
					auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
					for (const auto& key: driveEntry.downloadShards()) {
						auto downloadIter = downloadCache.find(key);
						auto& downloadEntry = downloadIter.get();
						if (!downloadEntry.isCloseInitiated()) {
							downloadEntry.setFinishPublished(true);
							downloadEntry.downloadApprovalInitiationEvent() = notification.TransactionHash;
						}
					}

					// Removing the drive from caches
					auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
					for (const auto& replicatorKey : driveEntry.replicators())
						replicatorCache.find(replicatorKey).get().drives().erase(notification.DriveKey);

					driveCache.remove(notification.DriveKey);

					// Assigning drive's former replicators to queued drives
					std::seed_seq seed(notification.TransactionHash.begin(), notification.TransactionHash.end());
					std::mt19937 rng(seed);
					utils::AssignReplicatorsToQueuedDrives(replicators, pDriveQueue, context, rng);
				}))
	}
}}