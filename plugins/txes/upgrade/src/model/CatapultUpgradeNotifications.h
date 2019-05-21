/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region catapult upgrade notification types

/// Defines an catapult upgrade notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_CATAPULT_UPGRADE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, CatapultUpgrade, DESCRIPTION, CODE)

	/// Catapult upgrade signer.
	DEFINE_CATAPULT_UPGRADE_NOTIFICATION(Signer_v1, 0x001, Validator);

	/// New catapult upgrade.
	DEFINE_CATAPULT_UPGRADE_NOTIFICATION(New_v1, 0x002, All);

#undef DEFINE_CATAPULT_UPGRADE_NOTIFICATION

	// endregion

	/// Notification of a catapult upgrade signer.
	template<VersionType version>
	struct CatapultUpgradeSignerNotification;

	template<>
	struct CatapultUpgradeSignerNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = CatapultUpgrade_Signer_v1_Notification;

	public:
		/// Creates a notification around \a signer.
		CatapultUpgradeSignerNotification(const Key& signer)
			: Notification(Notification_Type, sizeof(CatapultUpgradeSignerNotification<1>))
			, Signer(signer)
		{}

	public:
		/// Catapult upgrade signer.
		const Key& Signer;
	};

	/// Notification of a new catapult upgrade.
	template<VersionType version>
	struct CatapultUpgradeNotification;

	template<>
	struct CatapultUpgradeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = CatapultUpgrade_New_v1_Notification;

	public:
		/// Creates a notification around \a upgradeHeight and \a version.
		CatapultUpgradeNotification(const Height& upgradeHeight, bool isHeightRelative, const CatapultVersion& version)
			: Notification(Notification_Type, sizeof(CatapultUpgradeNotification<1>))
			, UpgradeHeight(upgradeHeight)
			, IsHeightRelative(isHeightRelative)
			, Version(version)
		{}

	public:
		/// Height to apply upgrade.
		Height UpgradeHeight;

		/// Whether applying height is absolute or relative to the current height.
		bool IsHeightRelative;

		/// Catapult version.
		CatapultVersion Version;
	};
}}
