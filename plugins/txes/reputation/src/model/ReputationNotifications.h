/**
*** Copyright (c) 2018-present,
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
#include "ModifyMultisigAccountAndReputationTransaction.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region reputation notification types

	/// Defines a reputation notification type.
	DEFINE_NOTIFICATION_TYPE(Observer, Reputation, Update, 0x0001);

	// endregion

	/// Notification of a reputation update.
	struct ReputationUpdateNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Reputation_Update_Notification;

	public:
		/// Creates a notification around \a signer, \a modificationsCount and \a pModifications.
		explicit ReputationUpdateNotification(
				const Key& signer,
				uint8_t modificationsCount,
				const CosignatoryModification* pModifications)
				: Notification(Notification_Type, sizeof(ReputationUpdateNotification))
				, Signer(signer)
				, ModificationsCount(modificationsCount)
				, ModificationsPtr(pModifications)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Number of modifications.
		uint8_t ModificationsCount;

		/// Const pointer to the first modification.
		const CosignatoryModification* ModificationsPtr;
	};
}}
