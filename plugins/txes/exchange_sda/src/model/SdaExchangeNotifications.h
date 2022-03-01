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

	/// Place SDA-SDA Offer.
	DEFINE_EXCHANGESDA_NOTIFICATION(Place_Sda_Offer_v1, 0x001, All);

	/// SDA-SDA Exchange.
	DEFINE_EXCHANGESDA_NOTIFICATION(Sda_Exchange_v1, 0x002, All);

	/// Remove SDA-SDA offer.
	DEFINE_EXCHANGESDA_NOTIFICATION(Remove_Sda_Offer_v1, 0x003, All);

#undef DEFINE_EXCHANGESDA_NOTIFICATION

	// endregion

	struct BaseOfferNotification : public Notification {
	public:
		BaseOfferNotification(
				NotificationType type,
				const Key& owner,
				uint8_t sdaOfferCount,
				const SdaOfferWithDuration* pSdaOffers)
			: Notification(type, sizeof(BaseOfferNotification))
			, Owner(owner)
			, SdaOfferCount(sdaOfferCount)
			, SdaOffersPtr(pSdaOffers)
		{}

	public:
		/// SDA-SDA Offer owner.
		const Key& Owner;

		/// SDA-SDA Offer count.
		uint8_t SdaOfferCount;

		/// SDA-SDA Offers.
		const SdaOfferWithDuration* SdaOffersPtr;
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
				uint8_t sdaOfferCount,
				const SdaOfferWithDuration* pOffers)
			: BaseOfferNotification(Notification_Type, owner, sdaOfferCount, pOffers)
		{}
	};

	/// Notification of an SDA-SDA exchange.
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
				uint8_t sdaOfferCount,
				const MatchedSdaOffer* pSdaOffers)
			: Notification(Notification_Type, sizeof(SdaExchangeNotification<1>))
			, Signer(signer)
			, SdaOfferCount(sdaOfferCount)
			, SdaOffersPtr(pSdaOffers)
		{}

	public:
		/// Transaction signer.
		const Key& Signer;

		/// Number of matched SDA-SDA offers.
		uint8_t SdaOfferCount;

		/// Matched SDA-SDA offers.
		const MatchedSdaOffer* SdaOffersPtr;
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
				uint8_t sdaOfferCount,
				const SdaOfferMosaic* pSdaOffers)
			: Notification(Notification_Type, sizeof(RemoveSdaOfferNotification<1>))
			, Owner(owner)
			, SdaOfferCount(sdaOfferCount)
			, SdaOffersPtr(pSdaOffers)
		{}

	public:
		/// SDA-SDA Offer owner.
		const Key& Owner;

		/// Mosaic count.
		uint8_t SdaOfferCount;

		/// Mosaic ids of SDA-SDA offers to remove.
		const SdaOfferMosaic* SdaOffersPtr;
	};
}}
