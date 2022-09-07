/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <catapult/utils/FileSize.h>
#include <catapult/utils/TimeSpan.h>
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"

#include <set>
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Storage plugin configuration settings.
	struct LiquidityProviderConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(liquidityprovider)

		/// Whether the plugin is enabled.
		bool Enabled;

		/// The public keys that are allowed to create liquidity providers
		utils::KeySet ManagerPublicKeys;

		/// The maximum history window size
		uint16_t MaxWindowSize;

		/// Describes the number of digits after dot for percent. For example, 2 allows the following notation 15.34%
		uint8_t PercentsDigitsAfterDot;

	private:
		LiquidityProviderConfiguration() = default;

	public:
		/// Creates an uninitialized storage configuration.
		static LiquidityProviderConfiguration Uninitialized();

		/// Loads an storage configuration from \a bag.
		static LiquidityProviderConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
