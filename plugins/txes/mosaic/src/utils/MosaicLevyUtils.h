/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MosaicLevy.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult {
	namespace  utils {
		/// compute if levy fee is valid
		validators::ValidationResult ValidateLevyTransaction(const Key& signer, MosaicId id, const validators::ValidatorContext& context);
		bool IsMosaicLevyFeeValid(const model::MosaicLevyRaw &levy);
		bool IsMosaicIdValid(MosaicId id,  const validators::ValidatorContext& context);
		bool IsAddressValid(catapult::UnresolvedAddress address, const validators::ValidatorContext& context);
	}
}