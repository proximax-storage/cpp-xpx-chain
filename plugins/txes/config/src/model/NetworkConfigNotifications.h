/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region network config notification types

/// Defines an network config notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_NETWORK_CONFIG_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, NetworkConfig, DESCRIPTION, CODE)

	/// Network config signer.
	DEFINE_NETWORK_CONFIG_NOTIFICATION(Signer_v1, 0x001, Validator);

	/// Blockchain config.
	DEFINE_NETWORK_CONFIG_NOTIFICATION(Network_Config_v1, 0x002, All);

#undef DEFINE_NETWORK_CONFIG_NOTIFICATION

	// endregion

	/// Notification of a network config signer.
	template<VersionType version>
	struct NetworkConfigSignerNotification;

	template<>
	struct NetworkConfigSignerNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = NetworkConfig_Signer_v1_Notification;

	public:
		/// Creates a notification around \a signer.
		NetworkConfigSignerNotification(const Key& signer)
			: Notification(Notification_Type, sizeof(NetworkConfigSignerNotification<1>))
			, Signer(signer)
		{}

	public:
		/// Network config signer.
		const Key& Signer;
	};

	/// Notification of a blockchain config.
	template<VersionType version>
	struct NetworkConfigNotification;

	template<>
	struct NetworkConfigNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = NetworkConfig_Network_Config_v1_Notification;

	public:
		/// Creates a notification around \a applyHeightDelta, \a networkConfigSize, \a networkConfigPtr,
		/// \a supportedEntityVersionsSize and \a supportedEntityVersionsPtr
		NetworkConfigNotification(const BlockDuration& applyHeightDelta, uint16_t networkConfigSize, const uint8_t* networkConfigPtr,
				uint16_t supportedEntityVersionsSize, const uint8_t* supportedEntityVersionsPtr)
			: Notification(Notification_Type, sizeof(NetworkConfigNotification<1>))
			, ApplyHeightDelta(applyHeightDelta)
			, BlockChainConfigSize(networkConfigSize)
			, BlockChainConfigPtr(networkConfigPtr)
			, SupportedEntityVersionsSize(supportedEntityVersionsSize)
			, SupportedEntityVersionsPtr(supportedEntityVersionsPtr)
		{}

	public:
		/// Height to apply config at.
		BlockDuration ApplyHeightDelta;

		/// Blockchain configuration size in bytes.
		uint16_t BlockChainConfigSize;

		/// Blockchain configuration data pointer.
		const uint8_t* BlockChainConfigPtr;

		/// Supported entity versions configuration size in bytes.
		uint16_t SupportedEntityVersionsSize;

		/// Supported entity versions configuration data pointer.
		const uint8_t* SupportedEntityVersionsPtr;
	};
}}
