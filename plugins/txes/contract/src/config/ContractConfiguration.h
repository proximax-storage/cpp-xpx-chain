/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/model/PluginConfiguration.h"
#include <stdint.h>

namespace catapult { namespace utils { class ConfigurationBag; } }

namespace catapult { namespace config {

	/// Contract plugin configuration settings.
	struct ContractConfiguration : public model::PluginConfiguration {
	public:
		/// Supported modify contract transaction versions.
		VersionSet ModifyContractTransactionSupportedVersions;

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
