/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/MosaicCache.h"
#include "src/cache/LevyCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicAddLevyNotification<1>;


	ValidationResult MosaicAddLevyValidatorDetail(const Notification& notification,
	                                                   const ValidatorContext& context) {
		
		/// 1. check if signer is eligible and mosaic ID to be added levy for is valid
		auto result = utils::IsLevyTransactionValid(notification.Signer, notification.MosaicId, context);
		if(result != ValidationResult::Success) return result;
		
		/// 2. levy is not available
		if( notification.Levy.Type == model::LevyType::None)
			return ValidationResult::Success;
		
		/// 3. check address validity
		if(! utils::IsAddressValid(notification.Levy.Recipient, context)) {
			return Failure_Mosaic_Recipient_Levy_Not_Exist;
		}
		
		/// 4. fees is valid
		if(!utils::IsMosaicLevyFeeValid(notification.Levy)){
			return Failure_Mosaic_Invalid_Levy_Fee;
		}
		
		/// 5. Check MosaicID if existing if it is not 0 (use default)
		if(notification.Levy.MosaicId != model::UnsetMosaicId) {
			if(!utils::IsMosaicIdValid(notification.Levy.MosaicId, context)) {
				return Failure_Mosaic_Id_Not_Found;
			}
		}
		
		/// 6. Check if levy is already in cache
		auto& cache = context.Cache.sub<cache::LevyCache>();
		auto iter = cache.find(notification.MosaicId);
		if(iter.tryGet()) {
			return Failure_Mosaic_Levy_Already_Exist;
		}
		
		return ValidationResult::Success;
	}

	DECLARE_STATEFUL_VALIDATOR(AddLevy, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(AddLevy, [](const auto& notification, const auto& context) {
			return MosaicAddLevyValidatorDetail(notification, context);
		});
	}
}}
