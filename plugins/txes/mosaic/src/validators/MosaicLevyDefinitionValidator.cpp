#include <src/catapult/model/Address.h>
#include "Validators.h"
#include "ActiveMosaicView.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/utils/MosaicLevyUtils.h"

namespace catapult { namespace validators {

		using Notification = model::MosaicDefinitionNotification<1>;

		ValidationResult MosaicLevyValidatorDetail(const model::MosaicDefinitionNotification<1>& notification,
		                                                  const ValidatorContext& context) {
			/// 1. levy is not available
			if( notification.Levy.Type == model::LevyType::None)
				return ValidationResult::Success;

			/// 2. check address validity
			if(! utils::IsAddressValid(notification.Levy.Recipient, context)) {
				return Failure_Mosaic_Recipient_Levy_Not_Exist;
			}

			/// 3. fees is valid
			if(!utils::IsMosaicLevyFeeValid(notification.Levy)){
				return Failure_Mosaic_Invalid_Levy_Fee;
			}

			/// 4. Check MosaicID if existing if it is not 0 (use default)
			if( notification.Levy.MosaicId != model::UnsetMosaicId) {
				if(!utils::IsMosaicIdValid(notification.Levy.MosaicId, context)) {
					return Failure_Mosaic_Id_Not_Found;
				}
			}
			
			return ValidationResult::Success;
		}

		DECLARE_STATEFUL_VALIDATOR(MosaicLevyDefinition, Notification)( ) {
			return MAKE_STATEFUL_VALIDATOR(MosaicLevyDefinition, [](const auto& notification, const auto& context) {
				return MosaicLevyValidatorDetail(notification, context);
			});
		}
	}}
