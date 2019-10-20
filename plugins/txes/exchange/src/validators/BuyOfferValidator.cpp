/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "ValidatorUtils.h"

namespace catapult { namespace validators {

	using Notification = model::BuyOfferNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(BuyOffer, Notification)(const config::ImmutableConfiguration& config) {
		return MAKE_STATEFUL_VALIDATOR(BuyOffer, [allowedMosaicIds = GetAllowedMosaicIds(config)](const auto& notification, const ValidatorContext& context) {
			return ValidateOffers(notification, context, allowedMosaicIds);
		})
	}
}}
