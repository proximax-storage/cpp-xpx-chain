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
#include "src/cache/MetadataCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using NotificationV1 = model::MetadataValueNotification<1>;
	using NotificationV2 = model::MetadataValueNotification<2>;

	template<typename NotificationType>
	ValidationResult MetadataValueValidator(const NotificationType& notification, const ValidatorContext& context) {
		auto& cache = context.Cache.sub<cache::MetadataCache>();
		auto metadataKey = state::ResolveMetadataKey(notification.PartialMetadataKey, notification.MetadataTarget, context.Resolvers);
		auto metadataIter = cache.find(metadataKey.uniqueKey());
		if (!metadataIter.tryGet()) {
			return notification.ValueSizeDelta == notification.ValueSize
					? ValidationResult::Success
					: Failure_Metadata_v2_Value_Size_Delta_Mismatch;
		}

		const auto& metadataValue = metadataIter.get().value();
		auto expectedCacheValueSize = notification.ValueSize;
		if (notification.ValueSizeDelta > 0)
			expectedCacheValueSize = static_cast<uint16_t>(expectedCacheValueSize - notification.ValueSizeDelta);

		if (expectedCacheValueSize != metadataValue.size())
			return Failure_Metadata_v2_Value_Size_Delta_Mismatch;

		if (notification.ValueSizeDelta >= 0)
			return ValidationResult::Success;

		auto requiredTrimCount = static_cast<uint16_t>(-notification.ValueSizeDelta);
		return metadataValue.canTrim({ notification.ValuePtr, notification.ValueSize }, requiredTrimCount)
					   ? ValidationResult::Success
					   : Failure_Metadata_v2_Value_Change_Irreversible;
	}

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(MetadataValueV1, NotificationV1, MetadataValueValidator<NotificationV1>)
	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(MetadataValueV2, NotificationV2, MetadataValueValidator<NotificationV2>)
}}
