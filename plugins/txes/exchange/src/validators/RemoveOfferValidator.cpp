/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/OfferCache.h"

namespace catapult { namespace validators {

	using Notification = model::RemoveOfferNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(RemoveOffer, [](const Notification& notification, const ValidatorContext& context) {
		if (notification.OfferCount == 0)
			return Failure_Exchange_No_Offers;

		auto& offerCache = context.Cache.sub<cache::OfferCache>();
		auto pHash = notification.OfferHashesPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pHash) {
			if (!offerCache.contains(*pHash))
				return Failure_Exchange_Offer_Doesnt_Exist;

			const auto& offerEntry = offerCache.find(*pHash).get();
			if (offerEntry.transactionSigner() != notification.Signer)
				return Failure_Exchange_Offer_Signer_Invalid;

			if (offerEntry.expiryHeight() < context.Height)
				return Failure_Exchange_Offer_Already_Removed;
		}

		return ValidationResult::Success;
	});
}}
