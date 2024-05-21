/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/Queue.h"
#include "src/utils/AVLTree.h"
#include "src/utils/StorageUtils.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::DriveClosureNotification<1>;
	using BigUint = boost::multiprecision::uint128_t;

	DECLARE_OBSERVER(DriveClosure, Notification)(const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
	 											 const std::vector<std::unique_ptr<StorageUpdatesListener>>& updatesListeners) {
		return MAKE_OBSERVER(DriveClosure, Notification, ([&](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

			CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Entered DriveClosure observer.";

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

			const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
			const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
			const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
			auto& statementBuilder = context.StatementBuilder();

			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto driveStateIter = accountStateCache.find(notification.DriveKey);
			auto& driveState = driveStateIter.get();
			auto driveOwnerIter = accountStateCache.find(driveEntry.owner());
			auto& driveOwnerState = driveOwnerIter.get();

			// The value will be used after removing drive entry. That's why the copy is needed
			const auto replicators = driveEntry.replicators();
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Replicator keys stored in driveEntry.replicators():";
			for (const auto& key : replicators)
				CATAPULT_LOG(debug) << "[DRIVE CLOSURE] - " << key;

			// Removing replicators from tree before must be performed before refunding
			auto replicatorKeyExtractor = [&storageMosaicId, &accountStateCache](const Key& key) {
				return std::make_pair(accountStateCache.find(key).get().Balances.get(storageMosaicId), key);
			};

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Creating replicatorTreeAdapter...";
			utils::AVLTreeAdapter<std::pair<Amount, Key>> replicatorTreeAdapter(
					context.Cache.template sub<cache::QueueCache>(),
							state::ReplicatorsSetTree,
							replicatorKeyExtractor,
							[&replicatorCache](const Key& key) -> state::AVLTreeNode {
						return replicatorCache.find(key).get().replicatorsSetNode();
						},
						[&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
						replicatorCache.find(key).get().replicatorsSetNode() = node;
					});
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Created replicatorTreeAdapter.";

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Removing replicators from the tree using replicatorTreeAdapter:";
			for (const auto& replicatorKey : replicators) {
				CATAPULT_LOG(debug) << "[DRIVE CLOSURE] - removing " << replicatorKey << "...";
				replicatorTreeAdapter.remove(replicatorKeyExtractor(replicatorKey));
			}
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Successfully removed replicators from the tree.";

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Refunding deposits to replicators...";
			// ...
			// for (const auto& replicatorKey : replicators) {
			//		auto replicatorIter = replicatorCache.find(replicatorKey);
			//		auto& replicatorEntry = replicatorIter.get();	// TODO: Possible place of an exception.
			//		...
			RefundDepositsOnDriveClosure(driveEntry.key(), replicators, context);
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Successfully refunded deposits to replicators.";

			// Making payments to replicators, if there is a pending data modification
			auto& activeDataModifications = driveEntry.activeDataModifications();

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Making payments to replicators, if there is a pending data modification.";
			if (!activeDataModifications.empty() && !replicators.empty()) {
				const auto& modificationSize = activeDataModifications.front().ExpectedUploadSizeMegabytes;
				const auto totalReplicatorAmount = Amount(
						modificationSize + // Download work
						modificationSize * (replicators.size() - 1) / replicators.size()); // Upload work
				for (const auto& replicatorKey : replicators) {
					liquidityProvider->debitMosaics(context, driveEntry.key(), replicatorKey,
													config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
													totalReplicatorAmount);

					// Adding Replicator Modification receipt.
					const auto receiptType = model::Receipt_Type_Drive_Closure_Replicator_Modification;
					const model::StorageReceipt receipt(receiptType, driveEntry.key(), replicatorKey,
														{ streamingMosaicId, currencyMosaicId }, totalReplicatorAmount);
					statementBuilder.addTransactionReceipt(receipt);
				}
			}

			const auto& pluginConfig =
					context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto paymentIntervalSeconds = pluginConfig.StorageBillingPeriod.seconds();

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Making payments to replicators for their participation time.";
			auto timeSinceLastPayment = (context.Timestamp - driveEntry.getLastPayment()).unwrap() / 1000;
			for (auto& [replicatorKey, info] : driveEntry.confirmedStorageInfos()) {
				auto replicatorIter = accountStateCache.find(replicatorKey);
				auto& replicatorState = replicatorIter.get();

				if (info.ConfirmedStorageSince) {
					info.TimeInConfirmedStorage =
							info.TimeInConfirmedStorage + context.Timestamp - *info.ConfirmedStorageSince;
				}
				BigUint driveSize = driveEntry.size();

				auto timeInConfirmedStorageSeconds = info.TimeInConfirmedStorage.unwrap() / 1000;

				if ( timeInConfirmedStorageSeconds > paymentIntervalSeconds ) {
					// It is possible if Drive Closure is executed in the block in which
					// the PeriodicStoragePayment would process the Drive
					timeInConfirmedStorageSeconds = paymentIntervalSeconds;
				}

				auto payment = Amount(((driveSize * timeInConfirmedStorageSeconds) / paymentIntervalSeconds)
											  .template convert_to<uint64_t>());
				liquidityProvider->debitMosaics(context, driveEntry.key(), replicatorKey,
												config::GetUnresolvedStorageMosaicId(context.Config.Immutable),
												payment);

				// Adding Replicator Participation receipt.
				const auto receiptType = model::Receipt_Type_Drive_Closure_Replicator_Participation;
				const model::StorageReceipt receipt(receiptType, driveEntry.key(), replicatorKey,
														{ storageMosaicId, currencyMosaicId }, payment);
				statementBuilder.addTransactionReceipt(receipt);
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
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Removing drive from the tree using driveTreeAdapter...";
		  	driveTreeAdapter.remove(notification.DriveKey);
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Successfully removed drive from the tree.";

			// Returning the rest to the drive owner
			const auto currencyRefundAmount = driveState.Balances.get(currencyMosaicId);
		  	const auto storageRefundAmount = driveState.Balances.get(storageMosaicId);
		  	const auto streamingRefundAmount = driveState.Balances.get(streamingMosaicId);

		  	if (currencyRefundAmount.unwrap() > 0) {
				driveState.Balances.debit(currencyMosaicId, currencyRefundAmount, context.Height);
				driveOwnerState.Balances.credit(currencyMosaicId, currencyRefundAmount, context.Height);

				// Adding Owner Refund receipt for currency.
				const auto receiptType = model::Receipt_Type_Drive_Closure_Owner_Refund;
			  	const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
													{ currencyMosaicId, currencyMosaicId }, currencyRefundAmount);
			  	statementBuilder.addTransactionReceipt(receipt);
		  	}

			if (storageRefundAmount.unwrap() > 0) {
				liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(),
												config::GetUnresolvedStorageMosaicId(context.Config.Immutable),
												storageRefundAmount);

				// Adding Owner Refund receipt for storage mosaic.
			  	const auto receiptType = model::Receipt_Type_Drive_Closure_Owner_Refund;
			  	const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
													{ storageMosaicId, currencyMosaicId }, storageRefundAmount);
			  	statementBuilder.addTransactionReceipt(receipt);
		  	}

			if (streamingRefundAmount.unwrap() > 0) {
				liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(),
												config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
												streamingRefundAmount);

		  		// Adding Owner Refund receipt for streaming mosaic.
				const auto receiptType = model::Receipt_Type_Drive_Closure_Owner_Refund;
				const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
													{ streamingMosaicId, currencyMosaicId }, streamingRefundAmount);
				statementBuilder.addTransactionReceipt(receipt);
			}

			// Removing the drive from queue, if present
			if (replicators.size() < driveEntry.replicatorCount()) {
				auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
				auto driveQueueIter = getPriorityQueueIter(priorityQueueCache, state::DrivePriorityQueueKey);
				auto& driveQueueEntry = driveQueueIter.get();

				CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Removing drive from priority queue...";
				driveQueueEntry.remove(notification.DriveKey);
				CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Successfully removed drive from priority queue.";
			}

			// Simulate publishing of finish download for all download channels
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Simulating publishing of finish download for all download channels.";
			auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
			utils::QueueAdapter<cache::DownloadChannelCache> downloadQueueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadCache);
			for (const auto& key: driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(key);
				auto& downloadEntry = downloadIter.get();
				if (!downloadEntry.isCloseInitiated()) {
					downloadEntry.setFinishPublished(true);
					downloadQueueAdapter.remove(key.array());
					downloadEntry.downloadApprovalInitiationEvent() = notification.TransactionHash;
				}
			}

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Updating listeners.";
			for (const auto& updateListener: updatesListeners) {
				updateListener->onDriveClosed(context, notification.DriveKey);
			}

			// Removing the drive from caches
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Removing the drive from replicator entries:";
			for (const auto& replicatorKey : replicators) {
				CATAPULT_LOG(debug) << "[DRIVE CLOSURE] - removing from " << replicatorKey << "...";
				auto replicatorIter = replicatorCache.find(replicatorKey);
				auto& replicatorEntry = replicatorIter.get();	// TODO: Possible place of an exception.
				replicatorEntry.drives().erase(notification.DriveKey);
			}

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Removing drive from the drive cache.";
			driveCache.remove(notification.DriveKey);

			// Assigning drive's former replicators to queued drives
			std::seed_seq seed(notification.TransactionHash.begin(), notification.TransactionHash.end());
			std::mt19937 rng(seed);

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Assigning drive's former replicators to queued drives...";
			// ...
			// for (const auto& replicatorKey : replicatorKeys) {
			//		auto replicatorIter = replicatorCache.find(replicatorKey);
			//		auto& replicatorEntry = replicatorIter.get();	// TODO: Possible place of an exception.
			//		...
			utils::AssignReplicatorsToQueuedDrives(replicators, context, rng);
		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Successfully assigned all drive's former replicators to queued drives.";

		  	CATAPULT_LOG(debug) << "[DRIVE CLOSURE] Reached end of the observer.";
		}))
	}
}}