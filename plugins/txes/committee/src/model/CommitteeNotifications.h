/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region committee notification types

/// Defines an committee notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_COMMITTEE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Committee, DESCRIPTION, CODE)

	/// Add harvester.
	DEFINE_COMMITTEE_NOTIFICATION(AddHarvester_v1, 0x001, All);

	/// Remove harvester.
	DEFINE_COMMITTEE_NOTIFICATION(RemoveHarvester_v1, 0x002, All);

#undef DEFINE_COMMITTEE_NOTIFICATION

	// endregion

	struct BaseHarvesterNotification : public Notification {
	public:
		/// Creates a notification around \a type and \a signer.
		BaseHarvesterNotification(NotificationType type, const Key& signer)
			: Notification(type, sizeof(BaseHarvesterNotification))
			, Signer(signer)
		{}

	public:
		/// Transaction signer.
		const Key& Signer;
	};

	/// Notification of adding a harvester to committee register.
	template<VersionType version>
	struct AddHarvesterNotification;

	template<>
	struct AddHarvesterNotification<1> : public BaseHarvesterNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Committee_AddHarvester_v1_Notification;

	public:
		/// Creates a notification around \a signer.
		AddHarvesterNotification(const Key& signer)
			: BaseHarvesterNotification(Notification_Type, signer)
		{}
	};

	/// Notification of adding a harvester to committee register.
	template<VersionType version>
	struct RemoveHarvesterNotification;

	template<>
	struct RemoveHarvesterNotification<1> : public BaseHarvesterNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Committee_RemoveHarvester_v1_Notification;

	public:
		/// Creates a notification around \a signer.
		RemoveHarvesterNotification(const Key& signer)
			: BaseHarvesterNotification(Notification_Type, signer)
		{}
	};
}}
