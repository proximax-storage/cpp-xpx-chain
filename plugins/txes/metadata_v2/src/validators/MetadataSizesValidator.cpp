/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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
#include "src/config/MetadataConfiguration.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::MetadataSizesNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(MetadataSizes, [](const auto& notification, const ValidatorContext& context) {
		// ValueSize cannot be zero because that implies cache entry value size is zero, which is not allowed
		if (0 == notification.ValueSize)
		  return Failure_Metadata_v2_Value_Too_Small;

		// following comparison works because it is comparing int32_t and uint16_t
		if (std::abs(static_cast<int32_t>(notification.ValueSizeDelta)) > notification.ValueSize)
		  return Failure_Metadata_v2_Value_Size_Delta_Too_Large;

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MetadataConfiguration>();
		return notification.ValueSize > pluginConfig.MaxValueSize ? Failure_Metadata_v2_Value_Too_Large : ValidationResult::Success;
	})
}}
