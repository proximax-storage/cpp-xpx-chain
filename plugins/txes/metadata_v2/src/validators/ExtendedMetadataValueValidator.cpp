/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/MetadataCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::MetadataValueNotification<2>;

	DEFINE_STATEFUL_VALIDATOR(ExtendedMetadataValue, ([](const Notification& notification, const ValidatorContext& context) {
		auto& cache = context.Cache.sub<cache::MetadataCache>();

		auto metadataKey = state::ResolveMetadataKey(notification.PartialMetadataKey, notification.MetadataTarget, context.Resolvers);
		auto metadataIter = cache.find(metadataKey.uniqueKey());
		if (!metadataIter.tryGet()) {
			return notification.ValueSizeDelta == notification.ValueSize
					? ValidationResult::Success
					: Failure_Metadata_v2_Value_Size_Delta_Mismatch;
		}

		const auto& metadataValue = metadataIter.get().value();
		if (metadataIter.get().isImmutable() && !metadataValue.empty())
			return Failure_Metadata_v2_Value_Is_Immutable;
	}))
}}
