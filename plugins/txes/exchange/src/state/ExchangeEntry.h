/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/exceptions.h"
#include "catapult/functions.h"
#include "src/model/Offer.h"
#include <cmath>
#include <map>

namespace catapult { namespace state {

	struct OfferBase {
	public:
		catapult::Amount Amount;
		catapult::Amount InitialAmount;
		catapult::Amount InitialCost;
		Height Deadline;

	public:
		double price() const;
	};

	struct SellOffer : public OfferBase {
	public:
		catapult::Amount cost(const catapult::Amount& amount) const;
		SellOffer& operator+=(const model::MatchedOffer& offer);
		SellOffer& operator-=(const model::MatchedOffer& offer);
	};

	struct BuyOffer : public OfferBase {
	public:
		catapult::Amount ResidualCost;

	public:
		catapult::Amount cost(const catapult::Amount& amount) const;
		BuyOffer& operator+=(const model::MatchedOffer& offer);
		BuyOffer& operator-=(const model::MatchedOffer& offer);
	};

	using SellOfferMap = std::map<MosaicId, SellOffer>;
	using BuyOfferMap = std::map<MosaicId, BuyOffer>;
	using ExpiredSellOfferMap = std::map<Height, SellOfferMap>;
	using ExpiredBuyOfferMap = std::map<Height, BuyOfferMap>;

	// Exchange entry.
	class ExchangeEntry {
	public:
		// Creates an exchange entry around \a owner.
		ExchangeEntry(const Key& owner)
			: m_owner(owner)
		{}

	public:
		static constexpr Height Invalid_Expiry_Height = Height(0);

	public:
		/// Gets the owner of the offers.
		const Key& owner() const {
			return m_owner;
		}

		/// Gets the sell offers.
		SellOfferMap& sellOffers() {
			return m_sellOffers;
		}

		/// Gets the sell offers.
		const SellOfferMap& sellOffers() const {
			return m_sellOffers;
		}

		/// Gets the buy offers.
		BuyOfferMap& buyOffers() {
			return m_buyOffers;
		}

		/// Gets the buy offers.
		const BuyOfferMap& buyOffers() const {
			return m_buyOffers;
		}

	public:
		/// Gets the height of the earliest expiring offer.
		Height minExpiryHeight() const;

		/// Gets the earliest height at which to prune expiring offers.
		Height minPruneHeight() const;

		/// Moves offer of \a type with \a mosaicId to expired offer buffer.
		void expireOffer(model::OfferType type, const MosaicId& mosaicId, const Height& height);

		/// Moves offer of \a type with \a mosaicId back from expired offer buffer.
		void unexpireOffer(model::OfferType type, const MosaicId& mosaicId, const Height& height);

		/// Moves offers to expired offer buffer if offer expiry height is equal \a height.
		template<typename TOfferMap, typename TExpiredOfferMap>
		void expireOffers(TOfferMap& offers, TExpiredOfferMap& expiredOffers, const Height& height, consumer<const typename TOfferMap::const_iterator&> action) {
			for (auto iter = offers.begin(); iter != offers.end();) {
				if (iter->second.Deadline == height) {
					auto &expiredOffersAtHeight = expiredOffers[height];
					if (expiredOffersAtHeight.count(iter->first))
						CATAPULT_THROW_RUNTIME_ERROR_2("offer with mosaic id already expired at height", iter->first, height);
					expiredOffersAtHeight.emplace(iter->first, iter->second);
					action(iter);
					iter = offers.erase(iter);
				} else {
					++iter;
				}
			}
		}

		void expireOffers(
			const Height& height,
			consumer<const BuyOfferMap::const_iterator&> buyOfferAction,
			consumer<const SellOfferMap::const_iterator&> sellOfferAction);

		/// Moves offers back from expired offer buffer if offer expiry height is equal \a height.
		template<typename TOfferMap, typename TExpiredOfferMap>
		void unexpireOffers(TOfferMap& offers, TExpiredOfferMap& expiredOffers, const Height& height, consumer<const typename TOfferMap::const_iterator&> action) {
			if (expiredOffers.count(height)) {
				auto& expiredOffersAtHeight = expiredOffers.at(height);
				for (auto iter = expiredOffersAtHeight.begin(); iter != expiredOffersAtHeight.end();) {
					if (offers.count(iter->first))
						CATAPULT_THROW_RUNTIME_ERROR_2("offer with mosaic id exists at height", iter->first, height);
					offers.emplace(iter->first, iter->second);
					action(offers.find(iter->first));
					iter = expiredOffersAtHeight.erase(iter);
				}
			}
		}

		void unexpireOffers(
			const Height& height,
			consumer<const BuyOfferMap::const_iterator&> buyOfferAction,
			consumer<const SellOfferMap::const_iterator&> sellOfferAction);

		bool offerExists(model::OfferType type, const MosaicId& mosaicId) const;
		void addOffer(model::OfferType type, const MosaicId& mosaicId, const model::OfferWithDuration* pOffer, const Height& deadline);
		void removeOffer(model::OfferType type, const MosaicId& mosaicId);
		state::OfferBase& getBaseOffer(model::OfferType type, const MosaicId& mosaicId);
		bool empty() const;

	public:
		/// Gets the expired sell offers.
		ExpiredSellOfferMap& expiredSellOffers() {
			return m_expiredSellOffers;
		}

		/// Gets the expired sell offers.
		const ExpiredSellOfferMap& expiredSellOffers() const {
			return m_expiredSellOffers;
		}

		/// Gets the expired buy offers.
		ExpiredBuyOfferMap& expiredBuyOffers() {
			return m_expiredBuyOffers;
		}

		/// Gets the expired buy offers.
		const ExpiredBuyOfferMap& expiredBuyOffers() const {
			return m_expiredBuyOffers;
		}

	private:
		Key m_owner;
		SellOfferMap m_sellOffers;
		BuyOfferMap m_buyOffers;
		ExpiredSellOfferMap m_expiredSellOffers;
		ExpiredBuyOfferMap m_expiredBuyOffers;
	};
}}
