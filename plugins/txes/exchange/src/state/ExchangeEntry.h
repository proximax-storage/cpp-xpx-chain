/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/exceptions.h"
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
		Height ExpiryHeight;
	};

	struct SellOffer : public OfferBase {
	public:
		catapult::Amount cost(const catapult::Amount& amount) const {
			auto cost = std::ceil(static_cast<double>(InitialCost.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialAmount.unwrap()));
			return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
		}

		SellOffer& operator+=(const model::MatchedOffer& offer) {
			Amount = Amount + offer.Mosaic.Amount;
			return *this;
		}

		SellOffer& operator-=(const model::MatchedOffer& offer) {
			if (offer.Mosaic.Amount > Amount)
				CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than mosaic amount", offer.Mosaic.Amount, Amount)
			Amount = Amount - offer.Mosaic.Amount;
			return *this;
		}
	};

	struct BuyOffer : public OfferBase {
	public:
		catapult::Amount ResidualCost;

	public:
		catapult::Amount cost(const catapult::Amount& amount) const {
			auto cost = std::floor(static_cast<double>(InitialCost.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialAmount.unwrap()));
			return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
		}

		BuyOffer& operator+=(const model::MatchedOffer& offer) {
			Amount = Amount + offer.Mosaic.Amount;
			ResidualCost = ResidualCost + offer.Cost;

			return *this;
		}

		BuyOffer& operator-=(const model::MatchedOffer& offer) {
			if (offer.Mosaic.Amount > Amount)
				CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than mosaic amount", offer.Mosaic.Amount, Amount)

			if (offer.Cost > ResidualCost)
				CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than residual cost", offer.Cost, ResidualCost)

			Amount = Amount - offer.Mosaic.Amount;
			ResidualCost = ResidualCost - offer.Cost;

			return *this;
		}
	};

	using SellOfferMap = std::map<MosaicId, SellOffer>;
	using BuyOfferMap = std::map<MosaicId, BuyOffer>;

	// Exchange entry.
	class ExchangeEntry {
	public:
		// Creates an exchange entry around \a owner.
		ExchangeEntry(const Key& owner)
			: m_owner(owner)
		{}

	public:

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

		/// Gets the height at which to remove the entry.
		const Height& expiryHeight() const {
			Height expiryHeight;
			std::for_each(m_sellOffers.begin(), m_sellOffers.end(), [&expiryHeight](const auto& pair) {
				if (expiryHeight < pair.second.ExpiryHeight)
					expiryHeight = pair.second.ExpiryHeight;
			});
			std::for_each(m_buyOffers.begin(), m_buyOffers.end(), [&expiryHeight](const auto& pair) {
				if (expiryHeight < pair.second.ExpiryHeight)
					expiryHeight = pair.second.ExpiryHeight;
			});
		}

	public:
		/// Returns \c true if at least one offer is active at \a height.
		bool isActive(catapult::Height height) const {
			for (const auto& pair : m_sellOffers) {
				if (height < pair.second.ExpiryHeight)
					return true;
			}
			for (const auto& pair : m_buyOffers) {
				if (height < pair.second.ExpiryHeight)
					return true;
			}

			return false;
		}

	private:
		Key m_owner;
		SellOfferMap m_sellOffers;
		BuyOfferMap m_buyOffers;
	};
}}
