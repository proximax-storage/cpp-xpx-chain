/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/Queue.h"
#include "src/utils/AVLTree.h"
#include "catapult/utils/StorageUtils.h"
#include <random>
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(PeriodicStoragePayment, Notification)(
			const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider,
			const std::vector<std::unique_ptr<StorageUpdatesListener>>& updatesListeners,
			const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(PeriodicStoragePayment, Notification, ([&liquidityProvider, &updatesListeners, pStorageState](const Notification& notification, ObserverContext& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			if (!pluginConfig.Enabled || context.Height < Height(2))
				return;

			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

			utils::QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);

			auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
			utils::QueueAdapter<cache::DownloadChannelCache> downloadQueueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadCache);

			if (queueAdapter.isEmpty()) {
				return;
			}

			auto paymentIntervalSeconds = pluginConfig.StorageBillingPeriod.seconds();

			// Creating unique eventHash for the observer
			auto eventHash = getStoragePaymentEventHash(notification.Timestamp, context.Config.Immutable.GenerationHash);

			auto maxIterations = queueAdapter.size();
			for (int i = 0; i < maxIterations; i++) {
				auto driveIter = driveCache.find(queueAdapter.front());
				auto& driveEntry = driveIter.get();

				auto timeSinceLastPaymentSeconds = (notification.Timestamp - driveEntry.getLastPayment()).unwrap() / 1000;
				if (timeSinceLastPaymentSeconds < paymentIntervalSeconds) {
					break;
				}

				queueAdapter.popFront();

				const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
				const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
				const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
				auto& statementBuilder = context.StatementBuilder();

				auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
				auto driveStateIter = accountStateCache.find(driveEntry.key());
				auto& driveState = driveStateIter.get();

				for (auto& [replicatorKey, info]: driveEntry.confirmedStorageInfos()) {
					if (info.ConfirmedStorageSince) {
						info.TimeInConfirmedStorage = info.TimeInConfirmedStorage + notification.Timestamp - *info.ConfirmedStorageSince;
						info.ConfirmedStorageSince = notification.Timestamp;
					}
					BigUint driveSize = driveEntry.size();

					auto timeInConfirmedStorageSeconds = info.TimeInConfirmedStorage.unwrap() / 1000;

					auto payment = Amount(((driveSize * timeInConfirmedStorageSeconds) / timeSinceLastPaymentSeconds).template convert_to<uint64_t>());
					liquidityProvider->debitMosaics(context, driveEntry.key(), replicatorKey,
													config::GetUnresolvedStorageMosaicId(context.Config.Immutable),
													payment);

					// Adding Replicator Participation receipt.
					const auto receiptType = model::Receipt_Type_Periodic_Payment_Replicator_Participation;
					const model::StorageReceipt receipt(receiptType, driveEntry.key(), replicatorKey,
														{ storageMosaicId, currencyMosaicId }, payment);
					statementBuilder.addTransactionReceipt(receipt);

					info.TimeInConfirmedStorage = Timestamp(0);
				}

				if (driveState.Balances.get(storageMosaicId).unwrap() >= driveEntry.size() * driveEntry.replicatorCount()) {

					// Drive Continues To Work
					driveEntry.setLastPayment(notification.Timestamp);
					queueAdapter.pushBack(driveEntry.entryKey());
				}
				else {
					// Drive is Closed

					// The value will be used after removing drive entry. That's why the copy is needed
					const auto replicators = driveEntry.replicators();

					auto keyExtractor = [=, &accountStateCache](const Key& key) {
						auto iter = accountStateCache.find(key);
						const auto& accountState = iter.get();
						return std::make_pair(accountState.Balances.get(storageMosaicId), key);
					};

					// Removing replicators from tree before must be performed before refunding
					utils::AVLTreeAdapter<std::pair<Amount, Key>> replicatorTreeAdapter(
							context.Cache.template sub<cache::QueueCache>(),
									state::ReplicatorsSetTree,
									keyExtractor,
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

					for (const auto& replicatorKey: replicators) {
						auto key = keyExtractor(replicatorKey);
						replicatorTreeAdapter.remove(key);
					}

					RefundDepositsOnDriveClosure(driveEntry.key(), replicators, context);

					// Making payments to replicators, if there is a pending data modification
					auto& activeDataModifications = driveEntry.activeDataModifications();

					if (!activeDataModifications.empty() && !replicators.empty()) {
						const auto& modificationSize = activeDataModifications.front().ExpectedUploadSizeMegabytes;
						const auto totalReplicatorAmount = Amount(
								modificationSize +	// Download work
								modificationSize * (replicators.size() - 1) / replicators.size());	// Upload work
						for (const auto& replicatorKey : replicators) {
							liquidityProvider->debitMosaics(context, driveEntry.key(), replicatorKey,
															config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
															totalReplicatorAmount);

							// Adding Replicator Modification receipt.
							const auto receiptType = model::Receipt_Type_Periodic_Payment_Replicator_Modification;
							const model::StorageReceipt receipt(receiptType, driveEntry.key(), replicatorKey,
																{ streamingMosaicId, currencyMosaicId }, totalReplicatorAmount);
							statementBuilder.addTransactionReceipt(receipt);
						}
					}

					// Returning the rest to the drive owner
					const auto refundStreamingAmount = driveState.Balances.get(streamingMosaicId);
					liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(),
													config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
													refundStreamingAmount);

					// Adding Owner Refund receipt for streaming mosaic.
					{
						const auto receiptType = model::Receipt_Type_Periodic_Payment_Owner_Refund;
						const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
															{ streamingMosaicId, currencyMosaicId }, refundStreamingAmount);
						statementBuilder.addTransactionReceipt(receipt);
					}

					const auto refundStorageAmount = driveState.Balances.get(storageMosaicId);
					liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(),
													config::GetUnresolvedStorageMosaicId(context.Config.Immutable),
													refundStorageAmount);

					// Adding Owner Refund receipt for storage mosaic.
					{
						const auto receiptType = model::Receipt_Type_Periodic_Payment_Owner_Refund;
						const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
															{ storageMosaicId, currencyMosaicId }, refundStorageAmount);
						statementBuilder.addTransactionReceipt(receipt);
					}

					// Simulate publishing of finish download for all download channels

					std::vector<std::shared_ptr<state::DownloadChannel>> downloadChannels;
					downloadChannels.reserve(driveEntry.downloadShards().size());
					for (const auto& key: driveEntry.downloadShards()) {
						auto downloadIter = downloadCache.find(key);
						auto& downloadEntry = downloadIter.get();
						if (!downloadEntry.isCloseInitiated()) {
							downloadEntry.setFinishPublished(true);
							downloadQueueAdapter.remove(key.array());
							downloadEntry.downloadApprovalInitiationEvent() = eventHash;

							auto pChannel = utils::GetDownloadChannel(pStorageState->replicatorKey(), downloadEntry);
							if (pChannel)
								downloadChannels.push_back(std::move(pChannel));
						}
					}

					if (!downloadChannels.empty())
						context.Notifications.push_back(std::make_unique<model::DownloadRewardServiceNotification<1>>(std::move(downloadChannels)));

					for (const auto& updateListener: updatesListeners)
						updateListener->onDriveClosed(context, driveEntry.key());

					// Removing the drive from queue, if present
					if (replicators.size() < driveEntry.replicatorCount()) {
						auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
						auto driveQueueIter = getPriorityQueueIter(priorityQueueCache, state::DrivePriorityQueueKey);
						auto& driveQueueEntry = driveQueueIter.get();

						driveQueueEntry.remove(driveEntry.key());
					}

					// Removing the drive from caches
					for (const auto& replicatorKey : replicators) {
						auto replicatorIter = replicatorCache.find(replicatorKey);
						if (pluginConfig.EnableCacheImprovement) {
							auto& replicatorEntry = replicatorIter.get();
							replicatorEntry.drives().erase(driveEntry.key());
						} else {
							auto replicatorEntry = replicatorIter.get();
							replicatorEntry.drives().erase(driveEntry.key());
						}
					}

					// The Drive is Removed, so we should make removal from verification tree
					utils::AVLTreeAdapter<Key> treeAdapter(
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
					treeAdapter.remove(driveEntry.key());

					driveCache.remove(driveEntry.key());

					// Assigning drive's former replicators to queued drives
					std::seed_seq seed(eventHash.begin(), eventHash.end());
					std::mt19937 rng(seed);
					auto updatedDrives = utils::AssignReplicatorsToQueuedDrives(pStorageState->replicatorKey(), replicators, context, rng);

					context.Notifications.push_back(std::make_unique<model::DrivesUpdateServiceNotification<1>>(std::move(updatedDrives), std::vector<Key>{ driveEntry.key() }, context.Timestamp));
				}
			}
        }))
	};
}}