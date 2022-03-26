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

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Storage plugin configuration settings.
	struct LiquidityProviderConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(liquidityprovider)

		/// Whether the plugin is enabled.
		bool Enabled;

	private:
		LiquidityProviderConfiguration() = default;

	public:
		/// Creates an uninitialized storage configuration.
		static LiquidityProviderConfiguration Uninitialized();

		/// Loads an storage configuration from \a bag.
		static LiquidityProviderConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
