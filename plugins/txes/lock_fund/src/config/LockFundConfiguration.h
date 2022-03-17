/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include <stdint.h>
#include <catapult/types.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Lock fund plugin configuration settings.
	struct LockFundConfiguration : public model::PluginConfiguration {
	public:
		DEFINE_CONFIG_CONSTANTS(lockfund)

	public:
		/// Blocks after which a request to unlock can be fulfilled
		BlockDuration MinRequestUnlockCooldown;

		/// Maximum transaction mosaics size.
		uint16_t MaxMosaicsSize;

	private:
		LockFundConfiguration() = default;

	public:
		/// Creates an uninitialized lock fund configuration.
		static LockFundConfiguration Uninitialized();

		/// Loads a lock fund configuration from \a bag.
		static LockFundConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
