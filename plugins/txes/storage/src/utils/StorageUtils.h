/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SwapOperation.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/observers/ObserverContext.h"
#include "src/cache/PriorityQueueCache.h"
#include "src/cache/ReplicatorKeyCollector.h"
#include "src/state/BcDriveEntry.h"
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
	state::PriorityQueueEntry& getPriorityQueueEntry(cache::PriorityQueueCache::CacheDeltaType&, const Key&);

	/// Calculates priority value of \a driveEntry. Used for the queue of drives with missing replicators.
	double CalculateDrivePriority(const state::BcDriveEntry&, const uint16_t&);

	/// Calculates amounts of storage and streaming deposit refunds of \a replicators
	/// with respect to the drive with \a driveKey, and transfers them to replicators' accounts.
	void RefundDepositsToReplicators(
			const Key&,
			const std::set<Key>&,
			const observers::ObserverContext&);

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
			const std::shared_ptr<cache::ReplicatorKeyCollector>&,
			const observers::ObserverContext&,
			std::mt19937&);

	/// Assigns each replicator from \a replicatorKeys to drives according to the drive priority queue.
	void AssignReplicatorsToQueuedDrives(
			const std::set<Key>&,
			const observers::ObserverContext&,
			std::mt19937&);
}}
