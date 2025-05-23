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
#include "catapult/crypto/Hashes.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace test {

	/// Creates a new notification with \a type.
	inline model::Notification CreateNotification(model::NotificationType type) {
		return model::Notification(type, sizeof(model::Notification));
	}

	/// Creates a placeholder block notification.
	inline model::BlockNotification<1> CreateBlockNotification() {
		// notice that notification Signer and Beneficiary will be garbage after this returns
		return model::BlockNotification<1>(Key(), Key(), Timestamp(), Difficulty(), 1, 1);
	}

	/// Creates a block notification around \a signer.
	inline model::BlockNotification<1> CreateBlockNotification(const Key& signer) {
		// notice that notification Beneficiary will be garbage after this returns
		return model::BlockNotification<1>(signer, Key(), Timestamp(), Difficulty(), 1, 1);
	}

	/// Creates a block notification around \a signer and \a beneficiary.
	inline model::BlockNotification<1> CreateBlockNotification(const Key& signer, const Key& beneficiary) {
		return model::BlockNotification<1>(signer, beneficiary, Timestamp(), Difficulty(), 1, 1);
	}

	/// Casts \a notification to a derived notification type.
	template<typename TNotification>
	const TNotification& CastToDerivedNotification(const model::Notification& notification) {
		if (sizeof(TNotification) > notification.Size)
			CATAPULT_THROW_INVALID_ARGUMENT_1("notification has incorrect size", utils::to_underlying_type(notification.Type));

		return static_cast<const TNotification&>(notification);
	}

	/// Calculates the hash of \a notification.
	inline Hash256 CalculateNotificationHash(const model::Notification& notification) {
		Hash256 notificationHash;
		crypto::Sha3_256({ reinterpret_cast<const uint8_t*>(&notification), notification.Size }, notificationHash);
		return notificationHash;
	}
}}
