/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Catapult upgrade plugin configuration settings.
	struct HelloConfiguration : public model::PluginConfiguration {
	public:
		uint16_t  messageCount;

	private:
		HelloConfiguration() = default;

	public:
		/// Creates an uninitialized catapult upgrade configuration.
		static HelloConfiguration Uninitialized();

		/// Loads an catapult upgrade configuration from \a bag.
		static HelloConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
