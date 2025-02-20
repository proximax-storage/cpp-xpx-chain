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
#include "MetadataValidatorShared.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	ValidationResult validateCommonData(uint16_t valueSize, int16_t valueSizeDelta, const state::MetadataValue& metadataValue, const uint8_t* valuePtr)
	{
		auto expectedCacheValueSize = valueSize;
		if (valueSizeDelta > 0)
			expectedCacheValueSize = static_cast<uint16_t>(expectedCacheValueSize - valueSizeDelta);

		if (expectedCacheValueSize != metadataValue.size())
			return Failure_Metadata_v2_Value_Size_Delta_Mismatch;

		if (valueSizeDelta >= 0)
			return ValidationResult::Success;

		auto requiredTrimCount = static_cast<uint16_t>(-valueSizeDelta);
		return metadataValue.canTrim({ valuePtr, valueSize }, requiredTrimCount)
					   ? ValidationResult::Success
					   : Failure_Metadata_v2_Value_Change_Irreversible;
	}
}}
