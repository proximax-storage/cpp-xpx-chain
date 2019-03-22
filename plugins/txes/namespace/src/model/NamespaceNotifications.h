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
#include "src/model/NamespaceConstants.h"
#include "src/model/NamespaceTypes.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region namespace notification types

/// Defines a namespace notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_NAMESPACE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Namespace, DESCRIPTION, CODE)

	/// Namespace name was provided.
	DEFINE_NAMESPACE_NOTIFICATION(Name, 0x0011, Validator);

	/// Namespace was registered.
	DEFINE_NAMESPACE_NOTIFICATION(Registration, 0x0012, Validator);

	/// Root namespace was registered.
	DEFINE_NAMESPACE_NOTIFICATION(Root_Registration, 0x0021, All);

	/// Child namespace was registered.
	DEFINE_NAMESPACE_NOTIFICATION(Child_Registration, 0x0022, All);

	/// Namespace rental fee has been sent.
	DEFINE_NAMESPACE_NOTIFICATION(Rental_Fee, 0x0030, Observer);

#undef DEFINE_NAMESPACE_NOTIFICATION

	// endregion

	/// Notification of a namespace name.
	struct NamespaceNameNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Name_Notification;

	public:
		/// Creates a notification around \a nameSize and \a pName given \a namespaceId and \a parentId.
		explicit NamespaceNameNotification(
				catapult::NamespaceId namespaceId,
				catapult::NamespaceId parentId,
				uint8_t nameSize,
				const uint8_t* pName)
				: Notification(Notification_Type, sizeof(NamespaceNameNotification))
				, NamespaceId(namespaceId)
				, ParentId(parentId)
				, NameSize(nameSize)
				, NamePtr(pName)
		{}

	public:
		/// Id of the namespace.
		catapult::NamespaceId NamespaceId;

		/// Id of the parent namespace.
		catapult::NamespaceId ParentId;

		/// Size of the name.
		uint8_t NameSize;

		/// Const pointer to the namespace name.
		const uint8_t* NamePtr;
	};

	/// Notification of a namespace registration.
	struct NamespaceNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Registration_Notification;

	public:
		/// Creates a notification around \a namespaceType.
		explicit NamespaceNotification(model::NamespaceType namespaceType)
				: Notification(Notification_Type, sizeof(NamespaceNotification))
				, NamespaceType(namespaceType)
		{}

	public:
		/// Type of the registered namespace.
		model::NamespaceType NamespaceType;
	};

	/// Notification of a root namespace registration.
	struct RootNamespaceNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Root_Registration_Notification;

	public:
		/// Creates a notification around \a signer, \a namespaceId and \a duration.
		explicit RootNamespaceNotification(const Key& signer, NamespaceId namespaceId, BlockDuration duration)
				: Notification(Notification_Type, sizeof(RootNamespaceNotification))
				, Signer(signer)
				, NamespaceId(namespaceId)
				, Duration(duration)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Id of the namespace.
		catapult::NamespaceId NamespaceId;

		/// Number of blocks for which the namespace should be valid.
		BlockDuration Duration;
	};

	/// Notification of a child namespace registration.
	struct ChildNamespaceNotification : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Child_Registration_Notification;

	public:
		/// Creates a notification around \a signer, \a namespaceId and \a parentId.
		explicit ChildNamespaceNotification(const Key& signer, NamespaceId namespaceId, NamespaceId parentId)
				: Notification(Notification_Type, sizeof(ChildNamespaceNotification))
				, Signer(signer)
				, NamespaceId(namespaceId)
				, ParentId(parentId)
		{}

	public:
		/// Signer.
		const Key& Signer;

		/// Id of the namespace.
		catapult::NamespaceId NamespaceId;

		/// Id of the parent namespace.
		catapult::NamespaceId ParentId;
	};

	// region rental fee

	/// Notification of a namespace rental fee.
	struct NamespaceRentalFeeNotification : public BasicBalanceNotification<NamespaceRentalFeeNotification> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Rental_Fee_Notification;

	public:
		/// Creates a notification around \a sender, \a recipient, \a mosaicId and \a amount.
		explicit NamespaceRentalFeeNotification(
				const Key& sender,
				const UnresolvedAddress& recipient,
				UnresolvedMosaicId mosaicId,
				catapult::Amount amount)
				: BasicBalanceNotification(sender, mosaicId, amount)
				, Recipient(recipient)
		{}

	public:
		/// Recipient.
		UnresolvedAddress Recipient;
	};

	// endregion
}}
