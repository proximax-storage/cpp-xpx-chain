/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/BuyOfferCache.h"

namespace catapult { namespace validators {

	using Notification = model::MatchedSellOfferNotification<1>;

	ValidationResult MatchedSellOfferValidator(const Notification& notification, const ValidatorContext& context) {
		return MatchedOfferValidator<Notification, cache::BuyOfferCache>(notification, context);
	}

	DEFINE_STATEFUL_VALIDATOR(MatchedSellOffer, MatchedSellOfferValidator);
}}
