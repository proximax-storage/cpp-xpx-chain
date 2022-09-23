/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include <stdint.h>
#include <catapult/types.h>
#include "plugins/services/globalstore/src/state/GlobalKeyGenerator.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Cache constants

	constexpr auto TotalStaked_GlobalKey = state::GenerateNewGlobalKeyFromLiteral("HarvestingMosaicTotalStakedValue");
	constexpr auto LockFundPluginInstalled_GlobalKey = state::GenerateNewGlobalKeyFromLiteral("LockFundPlugin_Installed");

	/// Lock fund plugin configuration settings.
	struct LockFundConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(lockfund)

	public:
		/// Whether the plugin is enabled.
		bool Enabled;
		
		/// Blocks after which a request to unlock can be fulfilled
		BlockDuration MinRequestUnlockCooldown;

		/// Maximum unlock requests each account can submit.

		uint MaxUnlockRequests;

		/// Maximum transaction mosaics size.
		uint16_t MaxMosaicsSize;

		/// Interval between each dock staking reward cycle.
		BlockDuration DockStakeRewardInterval;

	private:
		LockFundConfiguration() = default;

	public:
		/// Creates an uninitialized lock fund configuration.
		static LockFundConfiguration Uninitialized();

		/// Loads a lock fund configuration from \a bag.
		static LockFundConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
