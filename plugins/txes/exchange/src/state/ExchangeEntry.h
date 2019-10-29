/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <cmath>
#include <map>

namespace catapult { namespace state {

	struct SellOffer {
	public:
		catapult::Amount Amount;
		catapult::Amount InitialAmount;
		catapult::Amount InitialCost;
		Height Deadline;
		Height ExpiryHeight;

	public:
		catapult::Amount cost(const catapult::Amount& amount) const {
			auto cost = std::ceil(static_cast<double>(InitialCost.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialAmount.unwrap()));
			return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
		}
	};

	struct BuyOffer : SellOffer {
	public:
		catapult::Amount ResidualCost;

	public:
		catapult::Amount cost(const catapult::Amount& amount) const {
			auto cost = std::floor(static_cast<double>(InitialCost.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialAmount.unwrap()));
			return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
		}
	};

	using SellOfferMap = std::map<MosaicId, SellOffer>;
	using BuyOfferMap = std::map<MosaicId, BuyOffer>;

	// Offer entry.
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

		/// Checks whether all offers are fulfilled.
		bool fulfilled() {
			Amount sum(0);
			std::for_each(m_sellOffers.begin(), m_sellOffers.end(), [&sum](const auto& pair) {
				sum = sum + pair.second.Amount;
			});
			std::for_each(m_buyOffers.begin(), m_buyOffers.end(), [&sum](const auto& pair) {
				sum = sum + pair.second.Amount;
			});

			return (Amount(0) == sum);
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
