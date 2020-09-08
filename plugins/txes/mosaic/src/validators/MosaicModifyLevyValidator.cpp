/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicModifyLevyNotification<1>;


	ValidationResult MosaicModifyevyValidatorDetail(const Notification& notification,
	                                                   const ValidatorContext& context) {
		
		MosaicId baseMosaicId = context.Resolvers.resolve(notification.MosaicId);
		MosaicId levyMosaicId = context.Resolvers.resolve(notification.Levy.MosaicId);
		
		/// 1. check if signer is eligible and mosaic ID to be added levy for is valid
		auto result = utils::ValidateLevyTransaction(notification.Signer, baseMosaicId, context);
		if(result != ValidationResult::Success)
			return result;
		
		/// 2. levy type is invalid beyond allowed
		if( notification.Levy.Type <= model::LevyType::None
			|| notification.Levy.Type > model::LevyType::Percentile)
			return Failure_Mosaic_Invalid_Levy_Type;
		
		/// 3. check address validity
		if(! utils::IsAddressValid(notification.Levy.Recipient, context))
			return Failure_Mosaic_Recipient_Levy_Not_Exist;
		
		/// 4. fees is valid
		if(!utils::IsMosaicLevyFeeValid(notification.Levy))
			return Failure_Mosaic_Invalid_Levy_Fee;
		
		/// 5. Check MosaicID if valid
		if(!utils::IsMosaicIdValid(levyMosaicId, context))
			return Failure_Mosaic_Id_Not_Found;
		
		return ValidationResult::Success;
	}

	DECLARE_STATEFUL_VALIDATOR(ModifyLevy, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(ModifyLevy, [](const auto& notification, const auto& context) {
			return MosaicModifyevyValidatorDetail(notification, context);
		});
	}
}}
