/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include "catapult/types.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Service plugin configuration settings.
	struct ServiceConfiguration : public model::PluginConfiguration {
	public:
		/// Max files on drive at the moment
		uint16_t MaxFilesOnDrive;

		DEFINE_CONFIG_CONSTANTS(service)

		/// Verification fee in streaming units.
		Amount VerificationFee;

		/// Verification duration.
		BlockDuration VerificationDuration;

		/// Whether the plugin is enabled.
		bool Enabled;

	private:
		ServiceConfiguration() = default;

	public:
		/// Creates an uninitialized service configuration.
		static ServiceConfiguration Uninitialized();

		/// Loads an service configuration from \a bag.
		static ServiceConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
