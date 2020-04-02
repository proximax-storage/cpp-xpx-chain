
#include "Validators.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicModifyLevyNotification<1>;


	ValidationResult MosaicLModifyLevyValidatorDetail(const model::MosaicModifyLevyNotification<1>& notification,
	                                                   const ValidatorContext& context) {

		
		auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
		auto mosaicIter = mosaicCache.find(notification.MosaicId);
		if( !mosaicIter.tryGet()) {
			return Failure_Mosaic_Id_Not_Found;
		}

		auto& entry = mosaicIter.get();

		/// 0. check if signer allowed to modify levy
		if(entry.definition().owner() != notification.Signer) {
			return Failure_Mosaic_Ineligible_Signer;
		}

		/// 1. levy type switch to none?
		if( (notification.ModifyFlag & model::MosaicLevyModifyBitChangeType)
			&& notification.LevyInfo.Type == model::LevyType::None)
			return ValidationResult::Success;

		/// 2. check address validity
		if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeRecipient) {
			if(! utils::IsAddressValid(notification.LevyInfo.Recipient, context)) {
				return Failure_Mosaic_Recipient_Levy_Not_Exist;
			}
		}

		if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeLevyFee) {
			/// 3. fees is non zero
			if (!utils::IsMosaicLevyFeeValid(notification.LevyInfo)) {
				return Failure_Mosaic_Invalid_Levy_Fee;
			}
		}
		
		/// 4. Check levy MosaicID if valid
		if( notification.ModifyFlag & model::MosaicLevyModifyBitChangeMosaicId
			&& notification.LevyInfo.MosaicId != model::UnsetMosaicId) {
			if(!utils::IsMosaicIdValid(notification.LevyInfo.MosaicId, context)) {
				return Failure_Mosaic_Id_Not_Found;
			}
		}

		return ValidationResult::Success;
	}

	DECLARE_STATEFUL_VALIDATOR(ModifyMosaicLevy, Notification)( ) {
		return MAKE_STATEFUL_VALIDATOR(ModifyMosaicLevy, [](const auto& notification, const auto& context) {
			return MosaicLModifyLevyValidatorDetail(notification, context);
		});
	}
}}
