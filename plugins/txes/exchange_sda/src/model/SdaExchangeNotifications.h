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

	/// Remove SDA-SDA offer.
	DEFINE_EXCHANGESDA_NOTIFICATION(Remove_Sda_Offer_v1, 0x002, All);

#undef DEFINE_EXCHANGESDA_NOTIFICATION

	/// Notification of an SDA-SDA offer placing.
	template<VersionType version>
	struct PlaceSdaOfferNotification;

	template<>
	struct PlaceSdaOfferNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = ExchangeSda_Place_Sda_Offer_v1_Notification;

	public:
		PlaceSdaOfferNotification(
				const Key& signer,
				uint8_t sdaOfferCount,
				const SdaOfferWithOwnerAndDuration* pSdaOffers,
				const Key& owner)
			: Notification(Notification_Type, sizeof(PlaceSdaOfferNotification<1>))
			, Signer(signer)
			, SdaOfferCount(sdaOfferCount)
			, SdaOffersPtr(pSdaOffers)
			, OwnerOfSdaOfferToExchangeWith(owner)
		{}
	
	public:
		/// Transaction signer.
		const Key& Signer;

		/// Mosaic count.
		uint8_t SdaOfferCount;

		/// Own SDA-SDA Offers to exchange.
		const SdaOfferWithOwnerAndDuration* SdaOffersPtr;

		/// The person's SDA-SDA Offers to exchange with.
		const Key& OwnerOfSdaOfferToExchangeWith;
	};

	/// Notification of an SDA-SDA offer removing.
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
