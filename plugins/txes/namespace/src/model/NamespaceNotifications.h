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
#include "plugins/txes/namespace/src/model/NamespaceConstants.h"
#include "plugins/txes/namespace/src/model/NamespaceTypes.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region namespace notification types

/// Defines a namespace notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_NAMESPACE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Namespace, DESCRIPTION, CODE)

	/// Namespace name was provided.
	DEFINE_NAMESPACE_NOTIFICATION(Name_v1, 0x0011, Validator);

	/// Namespace was registered.
	DEFINE_NAMESPACE_NOTIFICATION(Registration_v1, 0x0012, Validator);

	/// Root namespace was registered.
	DEFINE_NAMESPACE_NOTIFICATION(Root_Registration_v1, 0x0021, All);

	/// Child namespace was registered.
	DEFINE_NAMESPACE_NOTIFICATION(Child_Registration_v1, 0x0022, All);

	/// Namespace rental fee has been sent.
	DEFINE_NAMESPACE_NOTIFICATION(Rental_Fee_v1, 0x0030, Observer);

	/// Namespace is required.
	DEFINE_NAMESPACE_NOTIFICATION(Required_v1, 0x0006, Validator);

#undef DEFINE_NAMESPACE_NOTIFICATION

	// endregion

	/// Notification of a namespace name.
	template<VersionType version>
	struct NamespaceNameNotification;

	template<>
	struct NamespaceNameNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Name_v1_Notification;

	public:
		/// Creates a notification around \a nameSize and \a pName given \a namespaceId and \a parentId.
		explicit NamespaceNameNotification(
				catapult::NamespaceId namespaceId,
				catapult::NamespaceId parentId,
				uint8_t nameSize,
				const uint8_t* pName)
				: Notification(Notification_Type, sizeof(NamespaceNameNotification<1>))
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
	template<VersionType version>
	struct NamespaceNotification;

	template<>
	struct NamespaceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Registration_v1_Notification;

	public:
		/// Creates a notification around \a namespaceType.
		explicit NamespaceNotification(model::NamespaceType namespaceType)
				: Notification(Notification_Type, sizeof(NamespaceNotification<1>))
				, NamespaceType(namespaceType)
		{}

	public:
		/// Type of the registered namespace.
		model::NamespaceType NamespaceType;
	};

	/// Notification of a root namespace registration.
	template<VersionType version>
	struct RootNamespaceNotification;

	template<>
	struct RootNamespaceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Root_Registration_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a namespaceId and \a duration.
		explicit RootNamespaceNotification(const Key& signer, NamespaceId namespaceId, BlockDuration duration)
				: Notification(Notification_Type, sizeof(RootNamespaceNotification<1>))
				, Signer(signer)
				, NamespaceId(namespaceId)
				, Duration(duration)
		{}

	public:
		/// Signer.
		const Key Signer;

		/// Id of the namespace.
		catapult::NamespaceId NamespaceId;

		/// Number of blocks for which the namespace should be valid.
		BlockDuration Duration;
	};

	/// Notification of a child namespace registration.
	template<VersionType version>
	struct ChildNamespaceNotification;

	template<>
	struct ChildNamespaceNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Child_Registration_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a namespaceId and \a parentId.
		explicit ChildNamespaceNotification(const Key& signer, NamespaceId namespaceId, NamespaceId parentId)
				: Notification(Notification_Type, sizeof(ChildNamespaceNotification<1>))
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
	template<VersionType version>
	struct NamespaceRentalFeeNotification;

	template<>
	struct NamespaceRentalFeeNotification<1> : public BasicBalanceNotification<NamespaceRentalFeeNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Rental_Fee_v1_Notification;

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

	// region namespace required

	/// Notification of a required namespace.
	template<VersionType version>
	struct NamespaceRequiredNotification;

	template<>
	struct NamespaceRequiredNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Namespace_Required_v1_Notification;

	public:
		/// Creates a notification around \a owner and \a namespaceId.
		NamespaceRequiredNotification(const Key& owner, NamespaceId namespaceId)
				: Notification(Notification_Type, sizeof(NamespaceRequiredNotification))
				, Owner(owner)
				, NamespaceId(namespaceId)
		{}

	public:
		/// Namespace owner.
		Key Owner;

		/// Namespace id.
		catapult::NamespaceId NamespaceId;
	};

	// endregion
}}
