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

	/// Streaming plugin configuration settings.
	struct StreamingConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(streaming)

		/// Whether the plugin is enabled.
		bool Enabled;

	private:
		StreamingConfiguration() = default;

	public:
		/// Creates an uninitialized Streaming configuration.
		static StreamingConfiguration Uninitialized();

		/// Loads an Streaming configuration from \a bag.
		static StreamingConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
