/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/utils/FileSize.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Network config plugin configuration settings.
	struct NetworkConfigConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(config)
		CATAPULT_DESTRUCTOR_CLEANUP_LOG(info, NetworkConfigConfiguration, ("Destroying NetworkConfigConfiguration"))

	public:
		/// Maximum blockchain config data size.
		utils::FileSize MaxBlockChainConfigSize;

		/// Maximum supported entity versions config data size.
		utils::FileSize MaxSupportedEntityVersionsSize;

	private:
		NetworkConfigConfiguration() = default;

	public:
		/// Creates an uninitialized network config plugin configuration.
		static NetworkConfigConfiguration Uninitialized();

		/// Loads an network config plugin configuration from \a bag.
		static NetworkConfigConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
