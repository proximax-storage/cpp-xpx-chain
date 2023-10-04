/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheUtils.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMosaicMetadataNotification_v1;

	DEFINE_STATEFUL_VALIDATOR(ModifyMosaicMetadata, [](const auto& notification, const ValidatorContext& context) {
		constexpr uint64_t mask = 1ull << 63;
		if (notification.MetadataId.unwrap() & mask)
			return Failure_Metadata_MosaicId_Malformed;

		const auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
		const auto& accountStateCache = context.Cache.template sub<cache::AccountStateCache>();

		auto it = mosaicCache.find(MosaicId(notification.MetadataId.unwrap()));

		if (!it.tryGet())
			return Failure_Metadata_Mosaic_Not_Found;

		if (!cache::FindActiveAccountKeyMatchBackwards(accountStateCache, notification.Signer, [owner = it.get().definition().owner()](Key relatedKey) {
				if(owner == relatedKey)
					return true;
				return false;
			}))
			return Failure_Metadata_Mosaic_Modification_Not_Permitted;

		return ValidationResult::Success;
	});
}}
