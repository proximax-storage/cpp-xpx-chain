/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region catapult config notification types

/// Defines an catapult config notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_CATAPULT_CONFIG_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, CatapultConfig, DESCRIPTION, CODE)

	/// Catapult config signer.
	DEFINE_CATAPULT_CONFIG_NOTIFICATION(Signer_v1, 0x001, Validator);

	/// Blockchain config.
	DEFINE_CATAPULT_CONFIG_NOTIFICATION(BlockChain_Config_v1, 0x002, All);

#undef DEFINE_CATAPULT_CONFIG_NOTIFICATION

	// endregion

	/// Notification of a catapult config signer.
	template<VersionType version>
	struct CatapultConfigSignerNotification;

	template<>
	struct CatapultConfigSignerNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = CatapultConfig_Signer_v1_Notification;

	public:
		/// Creates a notification around \a signer.
		CatapultConfigSignerNotification(const Key& signer)
			: Notification(Notification_Type, sizeof(CatapultConfigSignerNotification<1>))
			, Signer(signer)
		{}

	public:
		/// Catapult config signer.
		const Key& Signer;
	};

	/// Notification of a blockchain config.
	template<VersionType version>
	struct CatapultBlockChainConfigNotification;

	template<>
	struct CatapultBlockChainConfigNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = CatapultConfig_BlockChain_Config_v1_Notification;

	public:
		/// Creates a notification around \a applyHeightDelta, \a blockChainConfigSize and \a blockChainConfigPtr.
		CatapultBlockChainConfigNotification(const BlockDuration& applyHeightDelta, uint32_t blockChainConfigSize, const uint8_t* blockChainConfigPtr)
			: Notification(Notification_Type, sizeof(CatapultBlockChainConfigNotification<1>))
			, ApplyHeightDelta(applyHeightDelta)
			, BlockChainConfigSize(blockChainConfigSize)
			, BlockChainConfigPtr(blockChainConfigPtr)
		{}

	public:
		/// Height to apply config at.
		BlockDuration ApplyHeightDelta;

		/// Blockchain configuration size in bytes.
		uint32_t BlockChainConfigSize;

		/// Blockchain configuration data pointer.
		const uint8_t* BlockChainConfigPtr;
	};
}}
