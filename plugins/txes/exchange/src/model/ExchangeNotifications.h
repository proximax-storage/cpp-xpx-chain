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
	DEFINE_EXCHANGE_NOTIFICATION(Matched_Offer_v1, 0x002, All);

	/// Remove offer.
	DEFINE_EXCHANGE_NOTIFICATION(Matched_Remove_Offer_v1, 0x003, All);

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
				const Key& signer,
				const utils::ShortHash& transactionHash,
				OfferType offerType,
				const BlockDuration& duration,
				uint8_t offerCount,
				const Offer* pOffers)
			: Notification(Notification_Type, sizeof(OfferNotification<1>))
			, Signer(signer)
			, TransactionHash(transactionHash)
			, OfferType(offerType)
			, Duration(duration)
			, OfferCount(offerCount)
			, OffersPtr(pOffers)
		{}

	public:
		/// Offer transaction signer.
		const Key& Signer;

		/// Offer transaction hash.
		const utils::ShortHash& TransactionHash;

		/// The offer type.
		model::OfferType OfferType;

		/// Offer deadline.
		const BlockDuration& Duration;

		/// Offer count.
		uint8_t OfferCount;

		/// Offers to trade.
		const Offer* OffersPtr;
	};

	/// Notification of a matched offer.
	template<VersionType version>
	struct MatchedOfferNotification;

	template<>
	struct MatchedOfferNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Matched_Offer_v1_Notification;

	public:
		MatchedOfferNotification(
				const Key& signer,
				const utils::ShortHash& transactionHash,
				OfferType offerType,
				uint8_t matchedOfferCount,
				const MatchedOffer* pMatchedOffers,
				UnresolvedMosaicId currencyMosaicId,
				NotificationSubscriber& subscriber)
			: Notification(Notification_Type, sizeof(MatchedOfferNotification<1>))
			, Signer(signer)
			, TransactionHash(transactionHash)
			, OfferType(offerType)
			, MatchedOfferCount(matchedOfferCount)
			, MatchedOffersPtr(pMatchedOffers)
			, CurrencyMosaicId(currencyMosaicId)
			, Subscriber(subscriber)
		{}

	public:
		/// Offer transaction signer.
		const Key& Signer;

		/// Offer transaction hash.
		const utils::ShortHash& TransactionHash;

		/// The offer type.
		model::OfferType OfferType;

		/// Number of matched offers.
		uint8_t MatchedOfferCount;

		/// Matched offers.
		const MatchedOffer* MatchedOffersPtr;

		/// A notification subscriber.
		UnresolvedMosaicId CurrencyMosaicId;

		/// A notification subscriber.
		NotificationSubscriber& Subscriber;
	};

	/// Notification of a matched sell offer.
	template<VersionType version>
	struct RemoveOfferNotification;

	template<>
	struct RemoveOfferNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Matched_Remove_Offer_v1_Notification;

	public:
		RemoveOfferNotification(
				const Key& signer,
				uint8_t offerCount,
				const utils::ShortHash* pOfferHashes)
			: Notification(Notification_Type, sizeof(RemoveOfferNotification<1>))
			, Signer(signer)
			, OfferCount(offerCount)
			, OfferHashesPtr(pOfferHashes)
		{}

	public:
		/// Remove offer transaction signer.
		const Key& Signer;

		/// Offer count.
		uint8_t OfferCount;

		/// Hashes of offers to remove.
		const utils::ShortHash* OfferHashesPtr;
	};
}}
