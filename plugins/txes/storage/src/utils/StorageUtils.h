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
#include "src/state/BcDriveEntry.h"
#include <queue>

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

	using DrivePriority = std::pair<Key, double>;
	struct DriveQueueComparator {
		bool operator() (const DrivePriority& a, const DrivePriority& b) const {
			return a.second == b.second ? a.first < b.first : a.second < b.second;
		}
	};

	/// Calculates priority value of \a driveEntry. Used for the queue of drives with missing replicators.
	double CalculateDrivePriority(const state::BcDriveEntry&, const uint16_t&);
}}
