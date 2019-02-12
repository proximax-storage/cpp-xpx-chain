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

namespace catapult { namespace validators {

	using Notification = model::MosaicSupplyChangeNotification;

	namespace {
		constexpr bool IsValidDirection(model::MosaicSupplyChangeDirection direction) {
			return direction <= model::MosaicSupplyChangeDirection::Increase;
		}
	}

	DEFINE_STATELESS_VALIDATOR(MosaicSupplyChange, [](const auto& notification) {
		if (!IsValidDirection(notification.Direction))
			return Failure_Mosaic_Invalid_Supply_Change_Direction;

		return Amount() == notification.Delta
				? Failure_Mosaic_Invalid_Supply_Change_Amount
				: ValidationResult::Success;
	});
}}
