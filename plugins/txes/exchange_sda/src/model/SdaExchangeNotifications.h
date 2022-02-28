/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/SdaOffer.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace model {

	// region SDA-SDA exchange notification types

/// Defines an SDA-SDA exchange notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_EXCHANGESDA_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, ExchangeSda, DESCRIPTION, CODE)

	/// Place Offer.
	DEFINE_EXCHANGESDA_NOTIFICATION(Place_Sda_Offer_v1, 0x001, All);

	/// Exchange.
	DEFINE_EXCHANGESDA_NOTIFICATION(Sda_Exchange_v1, 0x002, All);

	/// Remove offer.
	DEFINE_EXCHANGESDA_NOTIFICATION(Remove_Sda_Offer_v1, 0x003, All);

#undef DEFINE_EXCHANGESDA_NOTIFICATION

	// endregion

	struct BaseOfferNotification : public Notification {
	public:
		BaseOfferNotification(
				NotificationType type,
				const Key& owner,
				uint8_t offerCount,
				const SdaOfferWithDuration* pOffers)
			: Notification(type, sizeof(BaseOfferNotification))
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
		const SdaOfferWithDuration* OffersPtr;
	};

	/// Notification of an SDA-SDA offer.
	template<VersionType version>
	struct SdaOfferNotification;

	template<>
	struct SdaOfferNotification<1> : public BaseOfferNotification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ExchangeSda_Place_Sda_Offer_v1_Notification;

	public:
		SdaOfferNotification(
				const Key& owner,
				uint8_t offerCount,
				const SdaOfferWithDuration* pOffers)
			: BaseOfferNotification(Notification_Type, owner, offerCount, pOffers)
		{}
	};

	/// Notification of an exchange.
	template<VersionType version>
	struct SdaExchangeNotification;

	template<>
	struct SdaExchangeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ExchangeSda_Sda_Exchange_v1_Notification;

	public:
		SdaExchangeNotification(
				const Key& signer,
				uint8_t offerCount,
				const MatchedSdaOffer* pOffers)
			: Notification(Notification_Type, sizeof(SdaExchangeNotification<1>))
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
		const MatchedSdaOffer* OffersPtr;
	};

	/// Notification of an offer removing.
	template<VersionType version>
	struct RemoveSdaOfferNotification;

	template<>
	struct RemoveSdaOfferNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ExchangeSda_Remove_Sda_Offer_v1_Notification;

	public:
		RemoveSdaOfferNotification(
				const Key& owner,
				uint8_t offerCount,
				const SdaOfferMosaic* pOffers)
			: Notification(Notification_Type, sizeof(RemoveSdaOfferNotification<1>))
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
		const SdaOfferMosaic* OffersPtr;
	};
}}
