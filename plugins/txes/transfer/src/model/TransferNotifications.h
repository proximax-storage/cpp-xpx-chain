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

	// region transfer notification types

/// Defines a transfer notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_TRANSFER_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Transfer, DESCRIPTION, CODE)

	/// Transfer was received with a message.
	DEFINE_TRANSFER_NOTIFICATION(Message, 0x001, Validator);

	/// Transfer was received with at least one mosaic.
	DEFINE_TRANSFER_NOTIFICATION(Mosaics, 0x002, Validator);

#undef DEFINE_TRANSFER_NOTIFICATION

	// endregion

	/// Notification of a transfer transaction with a message.
	struct TransferMessageNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Transfer_Message_Notification;

	public:
		/// Creates a notification around \a messageSize.
		explicit TransferMessageNotification(uint16_t messageSize)
				: Notification(Notification_Type, sizeof(TransferMessageNotification))
				, MessageSize(messageSize)
		{}

	public:
		/// Message size in bytes.
		uint16_t MessageSize;
	};

	/// Notification of a transfer transaction with mosaics.
	struct TransferMosaicsNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Transfer_Mosaics_Notification;

	public:
		/// Creates a notification around \a mosaicsCount and \a pMosaics.
		explicit TransferMosaicsNotification(uint8_t mosaicsCount, const UnresolvedMosaic* pMosaics)
				: Notification(Notification_Type, sizeof(TransferMosaicsNotification))
				, MosaicsCount(mosaicsCount)
				, MosaicsPtr(pMosaics)
		{}

	public:
		/// Number of mosaics.
		uint8_t MosaicsCount;

		/// Const pointer to the first mosaic.
		const UnresolvedMosaic* MosaicsPtr;
	};
}}
