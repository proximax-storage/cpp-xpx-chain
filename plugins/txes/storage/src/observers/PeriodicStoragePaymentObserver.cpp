/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/StorageStateImpl.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>
#include "src/utils/Queue.h"
#include "src/utils/AVLTree.h"
#include "src/catapult/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(PeriodicStoragePayment, Notification)() {
		return MAKE_OBSERVER(PeriodicStoragePayment, Notification, ([](const Notification& notification, ObserverContext& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			if (!pluginConfig.Enabled || context.Height < Height(2))
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

			utils::QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);

			if (queueAdapter.isEmpty()) {
				return;
			}

			auto paymentInterval = pluginConfig.StorageBillingPeriod.seconds();

			// Creating unique eventHash for the observer
			auto eventHash = getStoragePaymentEventHash(notification.Timestamp, context.Config.Immutable.GenerationHash);

			auto maxIterations = queueAdapter.size();
			for (int i = 0; i < maxIterations; i++) {
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
					driveEntry.setLastPayment(notification.Timestamp);
					queueAdapter.pushBack(driveEntry.entryKey());
				}
				else {
					// Drive is Closed

					// Making payments to replicators, if there is a pending data modification
					auto& activeDataModifications = driveEntry.activeDataModifications();

					// The value will be used after removing drive entry. That's why the copy is needed
					const auto replicators = driveEntry.replicators();

					if (!activeDataModifications.empty() && !replicators.empty()) {
						const auto& modificationSize = activeDataModifications.front().ExpectedUploadSizeMegabytes;
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

					auto keyExtractor = [=, &accountStateCache](const Key& key) {
						return std::make_pair(accountStateCache.find(key).get().Balances.get(storageMosaicId), key);
					};

					utils::AVLTreeAdapter<std::pair<Amount, Key>> replicatorTreeAdapter(
							context.Cache.template sub<cache::QueueCache>(),
							state::ReplicatorsSetTree,
							keyExtractor,
							[&replicatorCache](const Key& key) -> state::AVLTreeNode {
								return replicatorCache.find(key).get().replicatorsSetNode();
							},
							[&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
								replicatorCache.find(key).get().replicatorsSetNode() = node;
							});

					for (const auto& replicatorKey: replicators) {
						auto key = keyExtractor(replicatorKey);
						replicatorTreeAdapter.remove(key);
					}

					// Returning the rest to the drive owner
					const auto refundAmount = driveState.Balances.get(streamingMosaicId);
					driveState.Balances.debit(streamingMosaicId, refundAmount, context.Height);

					auto driveOwnerIter = accountStateCache.find(driveEntry.owner());
					auto& driveOwnerState = driveOwnerIter.get();
					driveOwnerState.Balances.credit(currencyMosaicId, refundAmount, context.Height);

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

					// Removing the drive from queue, if present
					if (replicators.size() < driveEntry.replicatorCount()) {
						auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
						auto& driveQueueEntry = getPriorityQueueEntry(priorityQueueCache, state::DrivePriorityQueueKey);

						driveQueueEntry.remove(driveEntry.key());
					}

					// Removing the drive from caches
					for (const auto& replicatorKey : replicators)
						replicatorCache.find(replicatorKey).get().drives().erase(driveEntry.key());

					// The Drive is Removed, so we should make removal from verification tree
					utils::AVLTreeAdapter<Key> treeAdapter(
							context.Cache.template sub<cache::QueueCache>(),
							state::DriveVerificationsTree,
							[](const Key& key) { return key; },
							[&driveCache](const Key& key) -> state::AVLTreeNode {
								return driveCache.find(key).get().verificationNode();
							},
							[&driveCache](const Key& key, const state::AVLTreeNode& node) {
								driveCache.find(key).get().verificationNode() = node;
							});
					treeAdapter.remove(driveEntry.key());

					driveCache.remove(driveEntry.key());

					// Assigning drive's former replicators to queued drives
					std::seed_seq seed(eventHash.begin(), eventHash.end());
					std::mt19937 rng(seed);
					utils::AssignReplicatorsToQueuedDrives(replicators, context, rng);
				}
			}
        }))
	};
}}