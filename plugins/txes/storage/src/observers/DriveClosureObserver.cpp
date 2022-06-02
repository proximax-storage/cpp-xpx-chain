/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/Queue.h"
#include "src/utils/AVLTree.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::DriveClosureNotification<1>;
	using BigUint = boost::multiprecision::uint128_t;

	DECLARE_OBSERVER(DriveClosure, Notification)() {
		return MAKE_OBSERVER(DriveClosure, Notification, ([](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

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
				const auto totalReplicatorAmount =
						Amount(modificationSize + // Download work
							   modificationSize * (replicators.size() - 1) / replicators.size()); // Upload work
				for (const auto& replicatorKey : replicators) {
					auto replicatorIter = accountStateCache.find(replicatorKey);
					auto& replicatorState = replicatorIter.get();
					driveState.Balances.debit(streamingMosaicId, totalReplicatorAmount, context.Height);
					replicatorState.Balances.credit(currencyMosaicId, totalReplicatorAmount, context.Height);
				}
			}

			const auto& pluginConfig =
					context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto paymentInterval = pluginConfig.StorageBillingPeriod.seconds();

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

			for (const auto& replicatorKey: driveEntry.replicators()) {
				auto key = keyExtractor(replicatorKey);
				replicatorTreeAdapter.remove(key);
			}

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
				driveState.Balances.debit(storageMosaicId, payment, context.Height);
				replicatorState.Balances.credit(currencyMosaicId, payment, context.Height);
			}

			// The Drive is Removed, so we should make removal from payment queue
			auto& queueCache = context.Cache.sub<cache::QueueCache>();
			utils::QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);
			queueAdapter.remove(driveEntry.entryKey());

			// The Drive is Removed, so we should make removal from verification tree
			utils::AVLTreeAdapter<Key> driveTreeAdapter(
					context.Cache.template sub<cache::QueueCache>(),
					state::DriveVerificationsTree,
					[](const Key& key) { return key; },
					[&driveCache](const Key& key) -> state::AVLTreeNode {
						return driveCache.find(key).get().verificationNode();
					},
					[&driveCache](const Key& key, const state::AVLTreeNode& node) {
						driveCache.find(key).get().verificationNode() = node;
					});

			for (const auto& replicatorKey: driveEntry.replicators()) {
				auto key = keyExtractor(replicatorKey);
				replicatorTreeAdapter.remove(key);
			}

			// Returning the rest to the drive owner
			const auto refundAmount = driveState.Balances.get(streamingMosaicId);
			driveState.Balances.debit(streamingMosaicId, refundAmount, context.Height);
			driveOwnerState.Balances.credit(currencyMosaicId, refundAmount, context.Height);

			// Removing the drive from queue, if present
			const auto replicators = driveEntry.replicators();
			if (replicators.size() < driveEntry.replicatorCount()) {
				auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
				auto& driveQueueEntry = getPriorityQueueEntry(priorityQueueCache, state::DrivePriorityQueueKey);

				driveQueueEntry.remove(notification.DriveKey);
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
			for (const auto& replicatorKey : driveEntry.replicators()) {
				auto replicatorIter = replicatorCache.find(replicatorKey);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.drives().erase(notification.DriveKey);

				replicatorTreeAdapter.remove(keyExtractor(replicatorKey));
				replicatorTreeAdapter.insert(replicatorKey);
			}

			driveCache.remove(notification.DriveKey);

			// Assigning drive's former replicators to queued drives
			std::seed_seq seed(notification.TransactionHash.begin(), notification.TransactionHash.end());
			std::mt19937 rng(seed);
			utils::AssignReplicatorsToQueuedDrives(replicators, context, rng);
		}))
	}
}}