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
#include "catapult/model/LockFundAction.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region transfer notification types

/// Defines a lockfund notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_LOCK_FUND_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, LockFund, DESCRIPTION, CODE)

	/// LockFund lock/unlock request was received.
	DEFINE_LOCK_FUND_NOTIFICATION(LockFund, 0x001, All);

	/// LockFund request to cancel funds unlocking was received.
	DEFINE_LOCK_FUND_NOTIFICATION(Cancel_Unlock, 0x002, All);

#undef DEFINE_TRANSFER_NOTIFICATION

	// endregion

	/// Notification of a lock fund lock transaction with a mosaicId and amount to lock.
	template<VersionType version>
	struct LockFundNotification;

	template<>
	struct LockFundNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockFund_LockFund_Notification;

	public:
		/// Creates a notification around \a messageSize.
		explicit LockFundNotification(Key sender, uint8_t mosaicsCount, BlockDuration duration, const UnresolvedMosaic* pMosaics, model::LockFundAction action)
				: Notification(Notification_Type, sizeof(LockFundNotification<1>))
				, Sender(sender)
				, MosaicsCount(mosaicsCount)
				, Duration(duration)
				, MosaicsPtr(pMosaics)
				, Action(action)
		{}

	public:
		/// Sender.
		const Key& Sender;

		/// Number of mosaics.
		uint8_t MosaicsCount;

		/// Number of mosaics.
		BlockDuration Duration;

		/// Const pointer to the first mosaic.
		const UnresolvedMosaic* MosaicsPtr;

		// Action
		LockFundAction Action;
	};

	/// Notification of a lock fund lock transaction with a mosaicId and amount to lock.
	template<VersionType version>
	struct LockFundCancelUnlockNotification;

	template<>
	struct LockFundCancelUnlockNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LockFund_Cancel_Unlock_Notification;

	public:
		/// Creates a notification around \a messageSize.
		explicit LockFundCancelUnlockNotification(Key sender, Height height)
				: Notification(Notification_Type, sizeof(LockFundCancelUnlockNotification<1>))
				, Sender(sender)
				, Height(height)
		{}

	public:
		/// Sender.
		const Key& Sender;

		/// Target unlock height
		Height Height;
	};
}}
