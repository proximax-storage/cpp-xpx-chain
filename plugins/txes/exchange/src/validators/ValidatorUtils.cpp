/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ValidatorUtils.h"
#include "catapult/config/ImmutableConfiguration.h"

namespace catapult { namespace validators {

	std::set<UnresolvedMosaicId> GetAllowedMosaicIds(const config::ImmutableConfiguration& config) {
		return {
			config::GetUnresolvedStorageMosaicId(config),
			config::GetUnresolvedStreamingMosaicId(config),
			config::GetUnresolvedReviewMosaicId(config),
			config::GetUnresolvedSuperContractMosaicId(config)
		};
	}
}}
