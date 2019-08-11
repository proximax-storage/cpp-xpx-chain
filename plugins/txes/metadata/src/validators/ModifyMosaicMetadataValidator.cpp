/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMosaicMetadataNotification_v1;

	DEFINE_STATEFUL_VALIDATOR(ModifyMosaicMetadata, [](const auto& notification, const ValidatorContext& context) {
		const auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
		auto it = mosaicCache.find(MosaicId(notification.MetadataId.unwrap()));

		if (!it.tryGet())
			return Failure_Metadata_Mosaic_Is_Not_Exist;

		if (it.get().definition().owner() != notification.Signer)
			return Failure_Metadata_Mosaic_Modification_Not_Permitted;

		return ValidationResult::Success;
	});
}}
