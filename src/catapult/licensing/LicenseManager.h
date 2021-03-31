/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LicensingConfiguration.h"
#include "catapult/types.h"
#include <memory>
#include <catapult/io/BlockStorageCache.h>

namespace catapult { namespace config { class BlockchainConfigurationHolder; } }

namespace catapult { namespace licensing {

	using BlockElementSupplier = std::function<std::shared_ptr<const model::BlockElement>(const Height& height)>;

	struct LicenseManager {
	public:
		virtual bool blockAllowedAt(const Height& height) = 0;
		virtual void setBlockElementSupplier(BlockElementSupplier supplier) = 0;
	};

	std::shared_ptr<LicenseManager> CreateDefaultLicenseManager(
		const std::string& licenseKey,
		const LicensingConfiguration& licensingConfig,
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
