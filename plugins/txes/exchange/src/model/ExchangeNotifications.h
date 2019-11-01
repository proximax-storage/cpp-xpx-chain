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

	/// Exchange.
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
				uint8_t offerCount,
				const OfferWithDuration* pOffers)
			: Notification(Notification_Type, sizeof(OfferNotification<1>))
			, Owner(owner)
			, OfferCount(offerCount)
			, OffersPtr(pOffers)
		{}

	public:
		/// Offer owner.
		const Key& Owner;

		/// Offer count.
		uint8_t OfferCount;

		/// Offers.
		const OfferWithDuration* OffersPtr;
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
				const Key& signer,
				uint8_t offerCount,
				const MatchedOffer* pOffers)
			: Notification(Notification_Type, sizeof(ExchangeNotification<1>))
			, Signer(signer)
			, OfferCount(offerCount)
			, OffersPtr(pOffers)
		{}

	public:
		/// Transaction signer.
		const Key& Signer;

		/// Number of matched offers.
		uint8_t OfferCount;

		/// Matched offers.
		const MatchedOffer* OffersPtr;
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
				uint8_t offerCount,
				const OfferMosaic* pOffers)
			: Notification(Notification_Type, sizeof(RemoveOfferNotification<1>))
			, Owner(owner)
			, OfferCount(offerCount)
			, OffersPtr(pOffers)
		{}

	public:
		/// Offer owner.
		const Key& Owner;

		/// Mosaic count.
		uint8_t OfferCount;

		/// Mosaic ids of offers to remove.
		const OfferMosaic* OffersPtr;
	};
}}
