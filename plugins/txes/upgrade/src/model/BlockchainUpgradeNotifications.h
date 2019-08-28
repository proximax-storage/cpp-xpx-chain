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
#define DEFINE_CATAPULT_UPGRADE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, BlockchainUpgrade, DESCRIPTION, CODE)

	/// Catapult upgrade signer.
	DEFINE_CATAPULT_UPGRADE_NOTIFICATION(Signer_v1, 0x001, Validator);

	/// Catapult upgrade version.
	DEFINE_CATAPULT_UPGRADE_NOTIFICATION(Version_v1, 0x002, All);

#undef DEFINE_CATAPULT_UPGRADE_NOTIFICATION

	// endregion

	/// Notification of a catapult upgrade signer.
	template<VersionType version>
	struct BlockchainUpgradeSignerNotification;

	template<>
	struct BlockchainUpgradeSignerNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = BlockchainUpgrade_Signer_v1_Notification;

	public:
		/// Creates a notification around \a signer.
		BlockchainUpgradeSignerNotification(const Key& signer)
			: Notification(Notification_Type, sizeof(BlockchainUpgradeSignerNotification<1>))
			, Signer(signer)
		{}

	public:
		/// Catapult upgrade signer.
		const Key& Signer;
	};

	/// Notification of a catapult upgrade.
	template<VersionType version>
	struct BlockchainUpgradeVersionNotification;

	template<>
	struct BlockchainUpgradeVersionNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = BlockchainUpgrade_Version_v1_Notification;

	public:
		/// Creates a notification around \a upgradePeriod and \a version.
		BlockchainUpgradeVersionNotification(const BlockDuration& upgradePeriod, const BlockchainVersion& version)
			: Notification(Notification_Type, sizeof(BlockchainUpgradeVersionNotification<1>))
			, UpgradePeriod(upgradePeriod)
			, Version(version)
		{}

	public:
		/// Height to force upgrade at.
		BlockDuration UpgradePeriod;

		/// Catapult version.
		BlockchainVersion Version;
	};
}}
