/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// DBRB plugin configuration settings.
	struct DbrbConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(dbrb)

		/// Whether the plugin is enabled.
		bool Enabled;

	private:
		DbrbConfiguration() = default;

	public:
		/// Creates an uninitialized DBRB configuration.
		static DbrbConfiguration Uninitialized();

		/// Loads a DBRB configuration from \a bag.
		static DbrbConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
