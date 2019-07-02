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

#include "Validators.h"
#include "catapult/constants.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "src/config/MosaicConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicPropertiesNotification<1>;

	namespace {
		constexpr bool IsValidFlags(model::MosaicFlags flags) {
			return flags <= model::MosaicFlags::All;
		}

		ValidationResult CheckOptionalProperties(const Notification& notification, BlockDuration maxMosaicDuration) {
			if (0 == notification.PropertiesHeader.Count)
				return ValidationResult::Success;

			if (notification.PropertiesHeader.Count >= 2)
				return Failure_Mosaic_Invalid_Property;

			const auto& property = *notification.PropertiesPtr;
			if (model::MosaicPropertyId::Duration != property.Id)
				return Failure_Mosaic_Invalid_Property;

			// note that Eternal_Artifact_Duration is default value and should not be specified explicitly
			auto duration = BlockDuration(property.Value);
			return maxMosaicDuration < duration || Eternal_Artifact_Duration == duration
					? Failure_Mosaic_Invalid_Duration
					: ValidationResult::Success;
		}
	}

	DECLARE_STATELESS_VALIDATOR(MosaicProperties, Notification)(const model::BlockChainConfiguration& blockChainConfig) {
		return MAKE_STATELESS_VALIDATOR(MosaicProperties, ([&blockChainConfig](const auto& notification) {
			if (!IsValidFlags(notification.PropertiesHeader.Flags))
				return Failure_Mosaic_Invalid_Flags;

			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::MosaicConfiguration>("catapult.plugins.mosaic");
			if (notification.PropertiesHeader.Divisibility > pluginConfig.MaxMosaicDivisibility)
				return Failure_Mosaic_Invalid_Divisibility;

			auto maxMosaicDuration = pluginConfig.MaxMosaicDuration.blocks(blockChainConfig.BlockGenerationTargetTime);
			return CheckOptionalProperties(notification, maxMosaicDuration);
		}));
	}
}}
