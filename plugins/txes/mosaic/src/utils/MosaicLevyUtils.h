#pragma once
#include "src/model/MosaicLevy.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult {
	namespace  utils {
		/// compute if levy fee is valid
		bool IsMosaicLevyFeeValid(const model::MosaicLevy &levy);
		bool IsMosaicIdValid(MosaicId id,  const validators::ValidatorContext& context);
		bool IsAddressValid(catapult::UnresolvedAddress address, const validators::ValidatorContext& context);
	}
}