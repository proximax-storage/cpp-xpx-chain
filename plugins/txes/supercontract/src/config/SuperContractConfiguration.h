/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Super contract plugin configuration settings.
	struct SuperContractConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(supercontract)

		/// Whether the plugin is enabled.
		bool Enabled;

		/// Maximum count of super contracts on a single drive.
		uint16_t MaxSuperContractsOnDrive;

	private:
		SuperContractConfiguration() = default;

	public:
		/// Creates an uninitialized Super contract configuration.
		static SuperContractConfiguration Uninitialized();

		/// Loads an Super contract configuration from \a bag.
		static SuperContractConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
