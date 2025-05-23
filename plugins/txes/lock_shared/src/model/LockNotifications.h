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
#include "catapult/model/Mosaic.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	/// Base for lock duration notification.
	template<typename TDerivedNotification>
	struct BaseLockDurationNotification : public Notification {
	public:
		/// Creates a notification around \a duration.
		explicit BaseLockDurationNotification(BlockDuration duration)
				: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
				, Duration(duration)
		{}

	public:
		/// Lock duration.
		BlockDuration Duration;
	};

	/// Base for lock transaction notification.
	template<typename TDerivedNotification>
	struct BaseLockNotification : public Notification {
	protected:
		/// Creates base lock notification around \a signer, \a mosaic and \a duration.
		BaseLockNotification(const Key& signer, const UnresolvedMosaic* pMosaics, uint8_t mosaicCount, BlockDuration duration)
				: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
				, Signer(signer)
				, MosaicsPtr(pMosaics)
				, MosaicCount(mosaicCount)
				, Duration(duration)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Locked mosaics.
		const UnresolvedMosaic* MosaicsPtr;

		/// Locked mosaics count.
		uint8_t MosaicCount;

		/// Lock duration.
		BlockDuration Duration;
	};
}}
