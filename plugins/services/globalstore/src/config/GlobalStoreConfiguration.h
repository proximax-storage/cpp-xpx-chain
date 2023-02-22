/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include <stdint.h>
#include "catapult/model/PluginConfiguration.h"
#include "catapult/config/ConfigConstants.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Account restriction plugin configuration settings.
	struct GlobalStoreConfiguration : public model::PluginConfiguration  {
	public:
		DEFINE_CONFIG_CONSTANTS(globalstore)
		CATAPULT_DESTRUCTOR_CLEANUP_LOG(info, GlobalStoreConfiguration, ("Destroying GlobalStoreConfiguration"))
	public:
		/// Whether the plugin is enabled.
		bool Enabled;


	private:
		GlobalStoreConfiguration() = default;

	public:
		/// Creates an uninitialized account restriction configuration.
		static GlobalStoreConfiguration Uninitialized();

		/// Loads account restriction configuration from \a bag.
		static GlobalStoreConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
