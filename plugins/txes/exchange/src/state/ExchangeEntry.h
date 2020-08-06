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
		// Creates an exchange entry around \a owner and \a version.
		ExchangeEntry(const Key& owner, VersionType version = Current_Version)
			: m_owner(owner)
			, m_version(version)
		{}

	public:
		static constexpr Height Invalid_Expiry_Height = Height(0);
		static constexpr VersionType Current_Version = 3;

	public:
		/// Gets the entry version.
		VersionType version() const {
			return m_version;
		}

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

		void expireOffers(
			const Height& height,
			consumer<const BuyOfferMap::const_iterator&> buyOfferAction,
			consumer<const SellOfferMap::const_iterator&> sellOfferAction);

		void unexpireOffers(
			const Height& height,
			consumer<const BuyOfferMap::const_iterator&> buyOfferAction,
			consumer<const SellOfferMap::const_iterator&> sellOfferAction);

		bool offerExists(model::OfferType type, const MosaicId& mosaicId) const;
		void addOffer(const MosaicId& mosaicId, const model::OfferWithDuration* pOffer, const Height& deadline);
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
		VersionType m_version;
		SellOfferMap m_sellOffers;
		BuyOfferMap m_buyOffers;
		ExpiredSellOfferMap m_expiredSellOffers;
		ExpiredBuyOfferMap m_expiredBuyOffers;
	};
}}
