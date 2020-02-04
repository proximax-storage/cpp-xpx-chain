/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/model/PluginConfiguration.h"
#include "catapult/plugins/PluginUtils.h"
#include "catapult/utils/BlockSpan.h"
#include "catapult/types.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Operation plugin configuration settings.
	struct OperationConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(operation)

	public:
		/// Whether the plugin is enabled.
		bool Enabled;

		/// Maximum number of blocks for which an operation can continue.
		utils::BlockSpan MaxOperationDuration;

	private:
		OperationConfiguration() = default;

	public:
		/// Creates an uninitialized lock configuration.
		static OperationConfiguration Uninitialized();

		/// Loads lock configuration from \a bag.
		static OperationConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
