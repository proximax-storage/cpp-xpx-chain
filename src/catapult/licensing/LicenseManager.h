/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LicensingConfiguration.h"
#include "catapult/types.h"
#include <memory>

namespace catapult { namespace config { class BlockchainConfigurationHolder; } }

namespace catapult { namespace licensing {

	struct LicenseManager {
	public:
		virtual bool blockGeneratingAllowedAt(const Height& height) const = 0;
		virtual bool blockConsumingAllowedAt(const Height& height) const = 0;
	};

	std::shared_ptr<LicenseManager> CreateDefaultLicenseManager(
		const std::string& licenseKey,
		const LicensingConfiguration& licensingConfig,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
