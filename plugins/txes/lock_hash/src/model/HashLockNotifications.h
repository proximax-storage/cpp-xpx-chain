/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "plugins/txes/lock_shared/src/model/LockNotifications.h"

namespace catapult { namespace model {

	// region lock hash notification types

/// Defines a lock hash notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_LOCKHASH_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, LockHash, DESCRIPTION, CODE)

	/// Lock hash duration.
	DEFINE_LOCKHASH_NOTIFICATION(Hash_Duration_v1, 0x0001, Validator);

	/// Lock mosaic.
	DEFINE_LOCKHASH_NOTIFICATION(Mosaic_v1, 0x0002, Validator);

	/// Lock hash.
	DEFINE_LOCKHASH_NOTIFICATION(Hash_v1, 0x0003, All);

#undef DEFINE_LOCKHASH_NOTIFICATION

	// endregion

	/// Notification of a hash lock mosaic.
	template<VersionType version>
	struct HashLockMosaicNotification;

	template<>
	struct HashLockMosaicNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockHash_Mosaic_v1_Notification;

	public:
		/// Creates a notification around \a mosaic.
		explicit HashLockMosaicNotification(UnresolvedMosaic mosaic)
				: Notification(Notification_Type, sizeof(HashLockMosaicNotification<1>))
				, Mosaic(mosaic)
		{}

	public:
		/// Locked mosaic.
		UnresolvedMosaic Mosaic;
	};

	/// Notification of a hash lock duration.
	template<VersionType version>
	struct HashLockDurationNotification;

	template<>
	struct HashLockDurationNotification<1> : public BaseLockDurationNotification<HashLockDurationNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockHash_Hash_Duration_v1_Notification;

	public:
		using BaseLockDurationNotification<HashLockDurationNotification<1>>::BaseLockDurationNotification;
	};

	/// Notification of a hash lock.
	template<VersionType version>
	struct HashLockNotification;

	template<>
	struct HashLockNotification<1> : public BaseLockNotification<HashLockNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockHash_Hash_v1_Notification;

	public:
		/// Creates hash lock notification around \a signer, \a mosaic, \a duration and \a hash.
		HashLockNotification(const Key& signer, const UnresolvedMosaic& mosaic, BlockDuration duration, const Hash256& hash)
				: BaseLockNotification(signer, &mosaic, 1, duration)
				, Hash(hash)
		{}

	public:
		/// Hash.
		const Hash256& Hash;
	};
}}
