/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/BuyOfferCache.h"
#include "src/cache/OfferDeadlineCache.h"
#include "src/cache/SellOfferCache.h"
#include "src/state/OfferDeadlineEntry.h"

namespace catapult { namespace validators {

	using Notification = model::RemoveOfferNotification<1>;

	template <typename TCache>
	ValidationResult ValidateOffers(
			const Key& transactionSigner,
			const utils::ShortHash* pHash,
			uint8_t count,
			TCache& cache,
			const ValidatorContext& context,
			bool buyOffers) {
		auto& offerDeadlineCache = context.Cache.sub<cache::OfferDeadlineCache>();
		const auto* pOfferDeadlineEntry = offerDeadlineCache.find(Height(0)).tryGet();

		for (uint8_t i = 0; i < count; ++i, ++pHash) {
			if (!cache.contains(*pHash))
				return Failure_Exchange_Offer_Doesnt_Exist;

			auto& entry = cache.find(*pHash).get();
			if (entry.transactionSigner() != transactionSigner)
				return Failure_Exchange_Offer_Signer_Invalid;

			if (pOfferDeadlineEntry) {
				{
					auto offerHeights = buyOffers ? pOfferDeadlineEntry->buyOfferHeights() : pOfferDeadlineEntry->sellOfferHeights();
					auto range = offerHeights.equal_range(context.Height);
					for (auto iter = range.first; iter != range.second; ++iter) {
						if (iter->second == *pHash)
							return Failure_Exchange_Offer_Already_Removed;
					}
				}

				{
					auto offerDeadlines = buyOffers ? pOfferDeadlineEntry->buyOfferDeadlines() : pOfferDeadlineEntry->sellOfferDeadlines();
					auto range = offerDeadlines.equal_range(entry.deadline());
					bool found = false;
					for (auto iter = range.first; iter != range.second; ++iter) {
						if (iter->second == *pHash) {
							found = true;
							break;
						}
					}
					if (!found)
						return Failure_Exchange_Offer_Already_Removed;
				}
			}
		}

		return ValidationResult::Success;
	}

	DEFINE_STATEFUL_VALIDATOR(RemoveOffer, [](const Notification& notification, const ValidatorContext& context) {
		auto& buyOfferCache = context.Cache.sub<cache::BuyOfferCache>();
		auto& sellOfferCache = context.Cache.sub<cache::SellOfferCache>();

		auto result = ValidateOffers(notification.Signer, notification.BuyOfferHashesPtr, notification.BuyOfferCount, buyOfferCache, context, true);
		if (IsValidationResultFailure(result))
			return result;

		result = ValidateOffers(notification.Signer, notification.SellOfferHashesPtr, notification.SellOfferCount, sellOfferCache, context, false);
		if (IsValidationResultFailure(result))
			return result;

		return ValidationResult::Success;
	});
}}
