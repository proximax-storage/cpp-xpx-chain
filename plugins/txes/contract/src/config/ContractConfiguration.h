/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Contract plugin configuration settings.
	struct ContractConfiguration {
	public:
		/// Minimum percentage of approval.
		uint8_t MinPercentageOfApproval;

		/// Minimum percentage of removal.
		uint8_t MinPercentageOfRemoval;

	private:
		ContractConfiguration() = default;

	public:
		/// Creates an uninitialized contract configuration.
		static ContractConfiguration Uninitialized();

		/// Loads an contract configuration from \a bag.
		static ContractConfiguration LoadFromBag(const utils::ConfigurationBag& bag);
	};
}}
