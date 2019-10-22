/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Exchange plugin configuration settings.
	struct ExchangeConfiguration : public model::PluginConfiguration {
	public:
		/// Whether the plugin is enabled.
		bool Enabled;

		/// Maximum offer duration.
		BlockDuration MaxOfferDuration;

	private:
		ExchangeConfiguration() = default;

	public:
		/// Creates an uninitialized service configuration.
		static ExchangeConfiguration Uninitialized();

		/// Loads an service configuration from \a bag.
		static ExchangeConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
