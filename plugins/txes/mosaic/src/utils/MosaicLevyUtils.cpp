/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/MosaicCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"
#include "src/validators/Validators.h"
#include "src/validators/ActiveMosaicView.h"

namespace catapult {
	namespace utils {
		
		bool IsMosaicLevyFeeValid(const model::MosaicLevyRaw &levy) {
			auto fee = levy.Fee;
			if (fee <= Amount(0))
				return false;

			/// Fee should not be greather than 100%
			if (levy.Type == model::LevyType::Percentile) {
				float pct = (levy.Fee.unwrap() / (float) model::MosaicLevyFeeDecimalPlace);
				if(pct > 100)
					return false;
			}
			
			return true;
		}
		
		bool IsMosaicIdValid(MosaicId id,  const validators::ValidatorContext& context)
		{
			// check that the mosaic is still active
			validators::ActiveMosaicView::FindIterator activeMosaicIter;
			validators::ActiveMosaicView activeMosaicView(context.Cache);
			auto result = activeMosaicView.tryGet(id, context.Height, activeMosaicIter);
			return IsValidationResultSuccess(result);
		}
		
		bool IsAddressValid(catapult::UnresolvedAddress address, const validators::ValidatorContext& context)
		{
			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			auto resolvedAddress = context.Resolvers.resolve(address);
			auto  accountStateKeyIter = cache.find(resolvedAddress);
			return accountStateKeyIter.tryGet();
		}
		
		validators::ValidationResult CheckLevyOperationAllowed(const Key& signer, MosaicId id,
			const validators::ValidatorContext& context)
		{
			if(!IsMosaicIdValid(id, context))
				return validators::Failure_Mosaic_Id_Not_Found;
			
			auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
			auto mosaicIter = mosaicCache.find(id);
			if( !mosaicIter.tryGet())
				return validators::Failure_Mosaic_Id_Not_Found;
			
			/// 0. check if signer allowed to modify levy
			auto& entry = mosaicIter.get();
			if(entry.definition().owner() != signer)
				return validators::Failure_Mosaic_Ineligible_Signer;
			
			return validators::ValidationResult::Success;
		}
	}
}
