/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {
		
	using Notification = model::MosaicRemoveLevyNotification<1>;
	
	ValidationResult MosaicRemoveLevyValidatorDetail(const Notification& notification,
	                                                     const ValidatorContext& context) {
		
		/// 1. check if signer is eligible and mosaic ID to be removed is valid
		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto result = utils::ValidateLevyTransaction(notification.Signer, mosaicId, context);
		if(result != ValidationResult::Success)
			return result;
		
		/// 2. Check if levy for this mosaicID exist
		auto& levyCache = context.Cache.sub<cache::LevyCache>();
		auto iter = levyCache.find(mosaicId);
		if(!iter.tryGet())
			return Failure_Mosaic_Levy_Not_Found;
		
		/// 3. Check if current levy is set
		auto entry = iter.get();
		if( entry.levy() == nullptr)
			return Failure_Mosaic_Current_Levy_Not_Set;
		
		return ValidationResult::Success;
	}
	
	DECLARE_STATEFUL_VALIDATOR(RemoveLevy, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(RemoveLevy, [](const auto& notification, const auto& context) {
			return MosaicRemoveLevyValidatorDetail(notification, context);
		});
	}
}}
