/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/Offer.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace model {

	// region exchange notification types

/// Defines an exchange notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_EXCHANGE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Exchange, DESCRIPTION, CODE)

	/// Offer.
	DEFINE_EXCHANGE_NOTIFICATION(Offer_v1, 0x001, All);

	/// Matched offer.
	DEFINE_EXCHANGE_NOTIFICATION(v1, 0x002, All);

	/// Remove offer.
	DEFINE_EXCHANGE_NOTIFICATION(Remove_Offer_v1, 0x003, All);

#undef DEFINE_EXCHANGE_NOTIFICATION

	// endregion

	/// Notification of an offer.
	template<VersionType version>
	struct OfferNotification;

	template<>
	struct OfferNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Offer_v1_Notification;
	public:
		OfferNotification(
				const Key& owner,
				const BlockDuration& duration,
				uint8_t sellOfferCount,
				const Offer* pSellOffers,
				uint8_t buyOfferCount,
				const Offer* pBuyOffers)
			: Notification(Notification_Type, sizeof(OfferNotification<1>))
			, Owner(owner)
			, Duration(duration)
			, SellOfferCount(sellOfferCount)
			, SellOffersPtr(pSellOffers)
			, BuyOfferCount(buyOfferCount)
			, BuyOffersPtr(pBuyOffers)
		{}

	public:
		/// Offer transaction owner.
		const Key& Owner;

		/// The offer type.
		model::OfferType OfferType;

		/// Offer deadline.
		const BlockDuration& Duration;

		/// Sell offer count.
		uint8_t SellOfferCount;

		/// Sell offers.
		const Offer* SellOffersPtr;

		/// Buy offer count.
		uint8_t BuyOfferCount;

		/// Buy offers.
		const Offer* BuyOffersPtr;
	};

	/// Notification of an exchange.
	template<VersionType version>
	struct ExchangeNotification;

	template<>
	struct ExchangeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_v1_Notification;

	public:
		ExchangeNotification(
				const Key& owner,
				uint8_t matchedOfferCount,
				const MatchedOffer* pMatchedOffers,
				UnresolvedMosaicId currencyMosaicId,
				NotificationSubscriber& subscriber)
			: Notification(Notification_Type, sizeof(ExchangeNotification<1>))
			, Owner(owner)
			, MatchedOfferCount(matchedOfferCount)
			, MatchedOffersPtr(pMatchedOffers)
			, CurrencyMosaicId(currencyMosaicId)
			, Subscriber(subscriber)
		{}

	public:
		/// Offer transaction owner.
		const Key& Owner;

		/// Number of matched offers.
		uint8_t MatchedOfferCount;

		/// Matched offers.
		const MatchedOffer* MatchedOffersPtr;

		/// A notification subscriber.
		UnresolvedMosaicId CurrencyMosaicId;

		/// A notification subscriber.
		NotificationSubscriber& Subscriber;
	};

	/// Notification of an offer removing.
	template<VersionType version>
	struct RemoveOfferNotification;

	template<>
	struct RemoveOfferNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Remove_Offer_v1_Notification;

	public:
		RemoveOfferNotification(
				const Key& owner,
				uint8_t mosaicCount,
				const MosaicId* pMosaic)
			: Notification(Notification_Type, sizeof(RemoveOfferNotification<1>))
			, Owner(owner)
			, MosaicCount(mosaicCount)
			, MosaicPtr(pMosaic)
		{}

	public:
		/// Remove offer transaction owner.
		const Key& Owner;

		/// Mosaic count.
		uint8_t MosaicCount;

		/// Hashes of offers to remove.
		const MosaicId* MosaicPtr;
	};
}}
