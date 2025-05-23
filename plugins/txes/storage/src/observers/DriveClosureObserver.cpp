/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
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

	DECLARE_OBSERVER(DriveClosure, Notification)(
			const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
			const std::vector<std::unique_ptr<StorageUpdatesListener>>& updatesListeners,
			const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(DriveClosure, Notification, ([&liquidityProvider, &updatesListeners, pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

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

			// Removing replicators from tree before must be performed before refunding
			auto replicatorKeyExtractor = [&storageMosaicId, &accountStateCache](const Key& key) {
				auto iter = accountStateCache.find(key);
				const auto& accountState = iter.get();
				return std::make_pair(accountState.Balances.get(storageMosaicId), key);
			};

			utils::AVLTreeAdapter<std::pair<Amount, Key>> replicatorTreeAdapter(
					context.Cache.template sub<cache::QueueCache>(),
							state::ReplicatorsSetTree,
							replicatorKeyExtractor,
							[&replicatorCache](const Key& key) -> state::AVLTreeNode {
						auto iter = replicatorCache.find(key);
						const auto& replicatorEntry = iter.get();
						return replicatorEntry.replicatorsSetNode();
						},
						[&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
						auto iter = replicatorCache.find(key);
						auto& replicatorEntry = iter.get();
						replicatorEntry.replicatorsSetNode() = node;
					});

			for (const auto& replicatorKey : replicators)
				replicatorTreeAdapter.remove(replicatorKeyExtractor(replicatorKey));

			RefundDepositsOnDriveClosure(driveEntry.key(), replicators, context);

			// Making payments to replicators, if there is a pending data modification
			auto& activeDataModifications = driveEntry.activeDataModifications();

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
						auto iter = driveCache.find(key);
						const auto& driveEntry = iter.get();
						return driveEntry.verificationNode();
					},
					[&driveCache](const Key& key, const state::AVLTreeNode& node) {
						auto iter = driveCache.find(key);
						auto& driveEntry = iter.get();
						driveEntry.verificationNode() = node;
					});
		  	driveTreeAdapter.remove(notification.DriveKey);

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

				driveQueueEntry.remove(notification.DriveKey);
			}

			// Simulate publishing of finish download for all download channels
			auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
			utils::QueueAdapter<cache::DownloadChannelCache> downloadQueueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadCache);
			std::vector<std::shared_ptr<state::DownloadChannel>> downloadChannels;
			downloadChannels.reserve(driveEntry.downloadShards().size());
			for (const auto& key: driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(key);
				auto& downloadEntry = downloadIter.get();
				if (!downloadEntry.isCloseInitiated()) {
					downloadEntry.setFinishPublished(true);
					downloadQueueAdapter.remove(key.array());
					downloadEntry.downloadApprovalInitiationEvent() = notification.TransactionHash;

					auto pChannel = utils::GetDownloadChannel(pStorageState->replicatorKey(), downloadEntry);
					if (pChannel)
						downloadChannels.push_back(std::move(pChannel));
				}
			}

			if (!downloadChannels.empty())
				context.Notifications.push_back(std::make_unique<model::DownloadRewardServiceNotification<1>>(std::move(downloadChannels)));

			for (const auto& updateListener: updatesListeners)
				updateListener->onDriveClosed(context, notification.DriveKey);

			// Removing the drive from caches
			for (const auto& replicatorKey : replicators) {
				auto replicatorIter = replicatorCache.find(replicatorKey);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.drives().erase(notification.DriveKey);
			}

			driveCache.remove(notification.DriveKey);

			// Assigning drive's former replicators to queued drives
			std::seed_seq seed(notification.TransactionHash.begin(), notification.TransactionHash.end());
			std::mt19937 rng(seed);
			auto updatedDrives = utils::AssignReplicatorsToQueuedDrives(pStorageState->replicatorKey(), replicators, context, rng);

			context.Notifications.push_back(std::make_unique<model::DrivesUpdateServiceNotification<1>>(std::move(updatedDrives), std::vector<Key>{ notification.DriveKey }, context.Timestamp));
		}))
	}
}}