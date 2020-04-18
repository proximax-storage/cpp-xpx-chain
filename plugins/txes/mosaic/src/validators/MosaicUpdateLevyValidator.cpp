/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicUpdateLevyNotification<1>;


	ValidationResult MosaicUpdateLevyValidatorDetail(const Notification& notification,
	                                                   const ValidatorContext& context) {
		
		/// 1. check if signer is eligible to modify levy
		auto result = utils::IsLevyTransactionValid(notification.Signer, notification.MosaicId, context);
		if(result != ValidationResult::Success) return result;
		
		/// 2. levy type switch to none?
		if( (notification.UpdateFlag & model::MosaicLevyModifyBitChangeType)
		    && notification.Levy.Type == model::LevyType::None)
			return ValidationResult::Success;
		
		/// 3. check address validity
		if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeRecipient) {
			if(! utils::IsAddressValid(notification.Levy.Recipient, context)) {
				return Failure_Mosaic_Recipient_Levy_Not_Exist;
			}
		}
		
		// 4. check fee validity
		if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeLevyFee) {
			/// 3. fees is non zero
			if (!utils::IsMosaicLevyFeeValid(notification.Levy)) {
				return Failure_Mosaic_Invalid_Levy_Fee;
			}
		}
		
		/// 5. Check levy MosaicID if valid
		if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeMosaicId
		    && notification.Levy.MosaicId != model::UnsetMosaicId) {
			if(!utils::IsMosaicIdValid(notification.Levy.MosaicId, context)) {
				return Failure_Mosaic_Id_Not_Found;
			}
		}
		
		return ValidationResult::Success;
	}

	DECLARE_STATEFUL_VALIDATOR(UpdateLevy, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(UpdateLevy, [](const auto& notification, const auto& context) {
			return MosaicUpdateLevyValidatorDetail(notification, context);
		});
	}
}}
