#include "src/cache/MosaicCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/MosaicLevy.h"

namespace catapult {
	namespace utils {
		
		///TODO: to be changed based on new fee implementation
		bool IsMosaicLevyFeeValid(const model::MosaicLevy &levy) {
			auto fee = levy.Fee;
			if (fee <= Amount(0))
				return false;

			/// Fee should not be greather than 100%
			if (levy.Type == model::LevyType::Percentile) {
				float pct = (levy.Fee.unwrap() / (float) model::MosaicLevyFeeDecimalPlace);
				if( pct > 100) {
					return false;
				}
			}
			
			return true;
		}
		
		bool IsMosaicIdValid(MosaicId id,  const validators::ValidatorContext& context)
		{
			auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();
			auto mosaicIter = mosaicCache.find(id);
			if( !mosaicIter.tryGet()) {
				return false;
			}
			return true;
		}
		
		bool IsAddressValid(catapult::UnresolvedAddress address, const validators::ValidatorContext& context)
		{
			auto& cache = context.Cache.sub<cache::AccountStateCache>();
			auto resolvedAddress = context.Resolvers.resolve(address);
			auto  accountStateKeyIter = cache.find(resolvedAddress);
			if( !accountStateKeyIter.tryGet() ) {
				return false;
			}
			return true;
		}
	}
}
