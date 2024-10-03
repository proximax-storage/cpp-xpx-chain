/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SwapOperation.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/observers/ObserverContext.h"
#include "src/cache/PriorityQueueCache.h"
#include "src/model/StorageReceiptType.h"
#include "src/state/BcDriveEntry.h"
#include "src/state/DownloadChannelEntry.h"
#include "src/catapult/observers/LiquidityProviderExchangeObserver.h"
#include <queue>
#include <random>

namespace catapult { namespace utils {

	/// Swap mosaics between \a sender and \a receiver.
	void SwapMosaics(const Key&, const Key&, const std::vector<model::UnresolvedMosaic>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Swap unresolved amount of mosaics between \a sender and \a receiver.
	void SwapMosaics(const Key&, const Key&, const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Swap mosaics on the \a account.
	void SwapMosaics(const Key&, const std::vector<model::UnresolvedMosaic>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Swap unresolved amount of mosaics on the \a account.
	void SwapMosaics(const Key&, const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Writes \a data to \a ptr one byte at a time. When done, \a ptr points to the past-the-last byte.
	template<typename TData>
	void WriteToByteArray(uint8_t*& ptr, const TData& data) {
		const auto pData = reinterpret_cast<const uint8_t*>(&data);
		ptr = std::copy(pData, pData + sizeof(data), ptr);
	}

	template<typename TIo, size_t Array_Size>
	void WriteToByteArray(uint8_t*& ptr, const std::array<uint8_t, Array_Size>& buffer) {
		ptr = std::copy(buffer.data(), buffer.data() + Array_Size, ptr);
	}

//	/// Creates priority queue for drives with correct comparator.
//	using DrivePriority = std::pair<Key, double>;
//	auto CreateDriveQueue() {
//		auto comparator = [](const DrivePriority& a, const DrivePriority& b) {
//			return a.second == b.second ? a.first < b.first : a.second < b.second;
//		};
//		return std::priority_queue<DrivePriority, std::vector<DrivePriority>, decltype(comparator)>(comparator);
//	};

	/// Gets priority queue entry with given \a queueKey from \a priorityQueueCache.
	/// If no entry was found, first creates one with an empty underlying priority queue.
	auto getPriorityQueueIter(cache::PriorityQueueCache::CacheDeltaType& cache, const Key& key) -> decltype(cache.find(key));

	/// Calculates priority value of \a driveEntry. Used for the queue of drives with missing replicators.
	double CalculateDrivePriority(const state::BcDriveEntry&, const uint16_t&);

	/// Gets void (zero key) account state. If necessary, creates it.
	auto getVoidState(const observers::ObserverContext& context) -> decltype(context.Cache.sub<cache::AccountStateCache>().find(Key()));

	/// Calculates amounts of deposit refunds of \a replicators in case of drive closure
	/// with respect to the drive with \a driveKey, and transfers them to replicators' accounts.
	void RefundDepositsOnDriveClosure(
			const Key&,
			const std::set<Key>&,
			observers::ObserverContext&);

	/// Calculates amounts of deposit refunds of \a replicators in case of replicator offboarding
	/// with respect to the drive with \a driveKey, and transfers them to replicators' accounts.
	void RefundDepositsOnOffboarding(
			const Key&,
			const std::set<Key>&,
			observers::ObserverContext&,
			const std::unique_ptr<observers::LiquidityProviderExchangeObserver>&);

	/// Performs actual offboarding of \a offboardingReplicators from the drive with \a driveKey;
	/// updates drive's data modification and download shards to keep them valid.
	void OffboardReplicatorsFromDrive(
			const Key&,
			const std::set<Key>&,
			const observers::ObserverContext&,
			std::mt19937&);

	/// Updates download and data modification shards of the \a driveEntry
	/// after a new replicator with \a replicatorKey has been added to the drive.
	void UpdateShardsOnAddedReplicator(
			state::BcDriveEntry&,
			const Key&,
			const observers::ObserverContext&,
			std::mt19937&);

	/// Assigns acceptable replicators from \a pKeyCollector to the drive with \a driveKey.
	void PopulateDriveWithReplicators(
			const Key&,
			observers::ObserverContext&,
			std::mt19937&);

	/// Assigns each replicator from \a replicatorKeys to drives according to the drive priority queue.
	std::vector<std::shared_ptr<state::Drive>> AssignReplicatorsToQueuedDrives(
			const Key&,
			const std::set<Key>&,
			observers::ObserverContext&,
			std::mt19937&);

	template<typename TDriveCache, typename TReplicatorCache, typename TDownloadChannelCache>
	std::shared_ptr<state::Drive> GetDrive(
			const Key& driveKey,
			const Key& localReplicatorKey,
			const Timestamp& timestamp,
			const TDriveCache& driveCache,
			const TReplicatorCache& replicatorCache,
			const TDownloadChannelCache& downloadChannelCache) {
		auto driveIter = driveCache.find(driveKey);
		const auto& driveEntry = driveIter.get();

		auto pDrive = std::make_shared<state::Drive>();
		pDrive->Id = driveKey;
		pDrive->Owner= driveEntry.owner();
		pDrive->RootHash = driveEntry.rootHash();
		pDrive->Size = driveEntry.size();
		pDrive->Replicators = utils::KeySet(driveEntry.replicators().begin(), driveEntry.replicators().end());

		for (const auto& replicatorKey : pDrive->Replicators) {
			auto replicatorIter = replicatorCache.find(replicatorKey);
			const auto& replicatorEntry = replicatorIter.get();
			auto driveInfoIter = replicatorEntry.drives().find(driveKey);
			if (driveInfoIter == replicatorEntry.drives().cend())
				CATAPULT_THROW_RUNTIME_ERROR_2("drive info not found", replicatorKey, driveKey)
			const auto& driveInfo = driveInfoIter->second;
			pDrive->ReplicatorInfo[replicatorKey] = state::ReplicatorDriveInfo{
				driveInfo.LastApprovedDataModificationId,
				driveInfo.InitialDownloadWorkMegabytes,
				driveInfo.LastCompletedCumulativeDownloadWorkBytes,
			};
		}

		for (const auto& [replicatorKey, shardInfo] : driveEntry.dataModificationShards()) {
			pDrive->ModificationShards[replicatorKey] = state::ModificationShard{
				shardInfo.ActualShardMembers,
				shardInfo.FormerShardMembers,
				shardInfo.OwnerUpload,
			};
		}

		auto dataModificationShardIter = driveEntry.dataModificationShards().find(localReplicatorKey);
		if (dataModificationShardIter != driveEntry.dataModificationShards().cend()) {
			const auto& shard = dataModificationShardIter->second;
			for (const auto& [key, _] : shard.ActualShardMembers)
				pDrive->DonatorShard.push_back(key);
		}

		for (const auto& [key, shard]: driveEntry.dataModificationShards()){
			const auto& actualShard = shard.ActualShardMembers;
			if (actualShard.find(localReplicatorKey) != actualShard.end())
				pDrive->RecipientShard.push_back(key);
		}

		for (const auto& channelId : driveEntry.downloadShards()) {
			auto channelIter = downloadChannelCache.find(channelId);
			const auto& channelEntry = channelIter.get();

			const auto& replicators = channelEntry.shardReplicators();
			if (replicators.find(localReplicatorKey) == replicators.end())
				continue;

			auto consumers = channelEntry.listOfPublicKeys();
			consumers.emplace_back(channelEntry.consumer());

			pDrive->DownloadChannels[channelId] = std::make_unique<state::DownloadChannel>(state::DownloadChannel{
				channelEntry.id(),
				driveKey,
				channelEntry.downloadSize(),
				consumers,
				{ replicators.begin(), replicators.end() },
				channelEntry.downloadApprovalInitiationEvent(),
			});
		}

		const auto& activeDataModifications = driveEntry.activeDataModifications();
		pDrive->DataModifications.reserve(activeDataModifications.size());
		for (const auto& modification: activeDataModifications) {
			pDrive->DataModifications.emplace_back(state::DataModification{
				modification.Id,
				driveKey,
				modification.DownloadDataCdi,
				modification.ExpectedUploadSizeMegabytes,
				modification.ActualUploadSizeMegabytes,
				modification.FolderName,
				modification.ReadyForApproval,
				modification.IsStream,
			});
		}

		for(const auto& modification: driveEntry.completedDataModifications()) {
			pDrive->CompletedModifications.push_back({
				modification.Id,
				modification.ApprovalState,
			});
		}

		auto replicatorInfoIter = pDrive->ReplicatorInfo.find(localReplicatorKey);
		pDrive->DownloadWorkBytes = (replicatorInfoIter != pDrive->ReplicatorInfo.end() ? replicatorInfoIter->second.LastCompletedCumulativeDownloadWorkBytes : 0);

		const auto& modifications = driveEntry.completedDataModifications();
		for (auto modificationIter = modifications.rbegin(); modificationIter != modifications.rend(); ++modificationIter) {
			if (modificationIter->ApprovalState == state::DataModificationApprovalState::Approved) {
				std::vector<Key> signers;
				for (const auto& state : driveEntry.confirmedStates()) {
					if (state.second == modificationIter->Id)
						signers.emplace_back(state.first);
				}

				pDrive->LastApprovedDataModificationPtr = std::make_unique<state::ApprovedDataModification>(state::ApprovedDataModification{
					modificationIter->Id,
					driveEntry.key(),
					modificationIter->DownloadDataCdi,
					modificationIter->ExpectedUploadSizeMegabytes,
					modificationIter->ActualUploadSizeMegabytes,
					modificationIter->FolderName,
					modificationIter->ReadyForApproval,
					modificationIter->IsStream,
					signers
				});
			}
		}

		if (driveEntry.verification()) {
			const auto& verification = *driveEntry.verification();
			pDrive->ActiveVerificationPtr = std::make_shared<state::DriveVerification>(state::DriveVerification{
				driveKey,
				verification.Duration,
				verification.expired(timestamp),
				verification.VerificationTrigger,
				driveEntry.lastModificationId(),
				verification.Shards,
			});
		}

		return pDrive;
	}

	std::unique_ptr<state::DownloadChannel> GetDownloadChannel(const Key& localReplicatorKey, const state::DownloadChannelEntry& channelEntry);
	std::shared_ptr<state::DriveVerification> GetDriveVerification(const state::BcDriveEntry& driveEntry, const Timestamp& timestamp);
}}
