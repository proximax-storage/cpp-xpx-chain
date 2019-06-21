/**
*** Copyright (c) 2016-present,
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

#include "ValidateHarvestingConfiguration.h"
#include "HarvestingConfiguration.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/HexParser.h"

namespace catapult { namespace harvesting {

	namespace {
		bool IsHarvestKeyValid(const HarvestingConfiguration& config) {
			return crypto::IsValidKeyString(config.HarvestKey) || (!config.IsAutoHarvestingEnabled && config.HarvestKey.empty());
		}
	}

	void ValidateHarvestingConfiguration(const HarvestingConfiguration& config) {
		if (!IsHarvestKeyValid(config))
			CATAPULT_THROW_AND_LOG_0(utils::property_malformed_error, "HarvestKey must be a valid private key")

		if (!crypto::IsValidKeyString(config.Beneficiary))
			CATAPULT_THROW_AND_LOG_0(utils::property_malformed_error, "Beneficiary must be a valid public key")
	}
}}
