/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/catapult/types.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Catapult upgrade plugin configuration settings.
	struct CatapultUpgradeConfiguration {
	public:
		/// Minimum duration in blocks before forcing update.
		BlockDuration MinUpgradePeriod;

	private:
		CatapultUpgradeConfiguration() = default;

	public:
		/// Creates an uninitialized catapult upgrade configuration.
		static CatapultUpgradeConfiguration Uninitialized();

		/// Loads an catapult upgrade configuration from \a bag.
		static CatapultUpgradeConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
