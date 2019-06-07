/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <stdint.h>
#include "catapult/model/PluginConfiguration.h"

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Account link plugin configuration settings.
	struct AccountLinkConfiguration : public model::PluginConfiguration {
	public:
		/// Supported account link transaction versions.
		VersionSet AccountLinkTransactionSupportedVersions;

	private:
		AccountLinkConfiguration() = default;

	public:
		/// Creates an uninitialized account link configuration.
		static AccountLinkConfiguration Uninitialized();

		/// Loads an account link configuration from \a bag.
		static AccountLinkConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
