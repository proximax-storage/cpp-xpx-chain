/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/OperationCache.h"

namespace catapult { namespace validators {

	using Notification = model::OperationMosaicNotification<1>;

	DEFINE_STATELESS_VALIDATOR(OperationMosaic, [](const auto& notification) {
		std::set<UnresolvedMosaicId> mosaicIds;
		auto pMosaic = notification.MosaicsPtr;
		for (auto i = 0u; i < notification.MosaicCount; ++i, ++pMosaic) {
			if (!mosaicIds.insert(pMosaic->MosaicId).second)
				return Failure_Operation_Mosaic_Redundant;
			if (Amount(0) == pMosaic->Amount)
				return Failure_Operation_Zero_Mosaic_Amount;
		}

		return ValidationResult::Success;
	})
}}
