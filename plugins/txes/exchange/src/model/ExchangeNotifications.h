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

	/// Buy offer.
	DEFINE_EXCHANGE_NOTIFICATION(Buy_Offer_v1, 0x001, All);

	/// Sell offer.
	DEFINE_EXCHANGE_NOTIFICATION(Sell_Offer_v1, 0x002, All);

	/// Matched buy offer.
	DEFINE_EXCHANGE_NOTIFICATION(Matched_Buy_Offer_v1, 0x003, All);

	/// Matched sell offer.
	DEFINE_EXCHANGE_NOTIFICATION(Matched_Sell_Offer_v1, 0x004, All);

	/// Remove offer.
	DEFINE_EXCHANGE_NOTIFICATION(Matched_Remove_Offer_v1, 0x004, All);

#undef DEFINE_EXCHANGE_NOTIFICATION

	// endregion

	/// A base offer notification.
	template<typename TDerivedNotification>
	struct BaseOfferNotification : public Notification {
	public:
		BaseOfferNotification(
				const Key& signer,
				const utils::ShortHash& transactionHash,
				const Timestamp& deadline,
				uint8_t offerCount,
				const Offer* pOffers)
			: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
			, Signer(signer)
			, TransactionHash(transactionHash)
			, Deadline(deadline)
			, OfferCount(offerCount)
			, OffersPtr(pOffers)
		{}

	public:
		/// Offer transaction signer.
		const Key& Signer;

		/// Offer transaction hash.
		const utils::ShortHash& TransactionHash;

		/// Offer deadline.
		const Timestamp& Deadline;

		/// Offer count.
		uint8_t OfferCount;

		/// Offers to trade.
		const Offer* OffersPtr;
	};

	/// Notification of a buy offer.
	template<VersionType version>
	struct BuyOfferNotification;

	template<>
	struct BuyOfferNotification<1> : public BaseOfferNotification<BuyOfferNotification<1>> {
	public:
		using Base = BaseOfferNotification<BuyOfferNotification<1>>;
		using Base::Base;

	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Buy_Offer_v1_Notification;
	};

	/// Notification of a sell offer.
	template<VersionType version>
	struct SellOfferNotification;

	template<>
	struct SellOfferNotification<1> : public BaseOfferNotification<SellOfferNotification<1>> {
	public:
		using Base = BaseOfferNotification<SellOfferNotification<1>>;
		using Base::Base;

	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Sell_Offer_v1_Notification;
	};

	/// A base offer notification.
	template<typename TDerivedNotification>
	struct BaseMatchedOfferNotification : public Notification {
	public:
		BaseMatchedOfferNotification(
				const Key& signer,
				const utils::ShortHash& transactionHash,
				uint8_t matchedOfferCount,
				const MatchedOffer* pMatchedOffers,
				UnresolvedMosaicId currencyMosaicId,
				NotificationSubscriber& subscriber)
			: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
			, Signer(signer)
			, TransactionHash(transactionHash)
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

		/// Number of matched offers.
		uint8_t MatchedOfferCount;

		/// Matched offers.
		const MatchedOffer* MatchedOffersPtr;

		/// A notification subscriber.
		UnresolvedMosaicId CurrencyMosaicId;

		/// A notification subscriber.
		NotificationSubscriber& Subscriber;
	};

	/// Notification of a matched buy offer.
	template<VersionType version>
	struct MatchedBuyOfferNotification;

	template<>
	struct MatchedBuyOfferNotification<1> : public BaseMatchedOfferNotification<MatchedBuyOfferNotification<1>> {
	public:
		using Base = BaseMatchedOfferNotification<MatchedBuyOfferNotification<1>>;
		using Base::Base;

	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Matched_Buy_Offer_v1_Notification;
	};

	/// Notification of a matched sell offer.
	template<VersionType version>
	struct MatchedSellOfferNotification;

	template<>
	struct MatchedSellOfferNotification<1> : public BaseMatchedOfferNotification<MatchedSellOfferNotification<1>> {
	public:
		using Base = BaseMatchedOfferNotification<MatchedSellOfferNotification<1>>;
		using Base::Base;

	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Exchange_Matched_Sell_Offer_v1_Notification;
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
				uint8_t buyOfferCount,
				const utils::ShortHash* pBuyOfferHashes,
				uint8_t sellOfferCount,
				const utils::ShortHash* pSellOfferHashes)
			: Notification(Notification_Type, sizeof(RemoveOfferNotification<1>))
			, Signer(signer)
			, BuyOfferCount(buyOfferCount)
			, BuyOfferHashesPtr(pBuyOfferHashes)
			, SellOfferCount(sellOfferCount)
			, SellOfferHashesPtr(pSellOfferHashes)
		{}

	public:
		/// Remove offer transaction signer.
		const Key& Signer;

		/// Buy offer count.
		uint8_t BuyOfferCount;

		/// Hashes of buy offers to remove.
		const utils::ShortHash* BuyOfferHashesPtr;

		/// Sell offer count.
		uint8_t SellOfferCount;

		/// Hashes of sell offers to remove.
		const utils::ShortHash* SellOfferHashesPtr;
	};
}}
