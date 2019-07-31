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

	/// Catapult upgrade version.
	DEFINE_CATAPULT_UPGRADE_NOTIFICATION(Version_v1, 0x002, All);

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

	/// Notification of a catapult upgrade.
	template<VersionType version>
	struct CatapultUpgradeVersionNotification;

	template<>
	struct CatapultUpgradeVersionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = CatapultUpgrade_Version_v1_Notification;

	public:
		/// Creates a notification around \a upgradePeriod and \a version.
		CatapultUpgradeVersionNotification(const BlockDuration& upgradePeriod, const CatapultVersion& version)
			: Notification(Notification_Type, sizeof(CatapultUpgradeVersionNotification<1>))
			, UpgradePeriod(upgradePeriod)
			, Version(version)
		{}

	public:
		/// Height to force upgrade at.
		BlockDuration UpgradePeriod;

		/// Catapult version.
		CatapultVersion Version;
	};
}}
