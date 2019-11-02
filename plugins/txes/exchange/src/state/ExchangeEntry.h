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
		bool Expired;
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

		/// Gets the height of the earliest expiring offer.
		Height minExpiryHeight() const {
			if (empty())
				return Invalid_Expiry_Height;

			Height expiryHeight = Height(std::numeric_limits<Height::ValueType>::max());
			std::for_each(m_sellOffers.begin(), m_sellOffers.end(), [&expiryHeight](const auto& pair) {
				if (expiryHeight > pair.second.ExpiryHeight)
					expiryHeight = pair.second.ExpiryHeight;
			});
			std::for_each(m_buyOffers.begin(), m_buyOffers.end(), [&expiryHeight](const auto& pair) {
				if (expiryHeight > pair.second.ExpiryHeight)
					expiryHeight = pair.second.ExpiryHeight;
			});

			return expiryHeight;
		}

		/// Marks offers as expired if expiry height is less or equal \a height.
		void markExpiredOffers(const Height& height) {
			std::for_each(m_sellOffers.begin(), m_sellOffers.end(), [&height](auto& pair) {
				pair.second.Expired = (height >= pair.second.ExpiryHeight);
			});
			std::for_each(m_buyOffers.begin(), m_buyOffers.end(), [&height](auto& pair) {
				pair.second.Expired = (height >= pair.second.ExpiryHeight);
			});
		}

		bool offerExists(model::OfferType type, const MosaicId& mosaicId) const {
			if (model::OfferType::Buy == type) {
				return m_buyOffers.count(mosaicId);
			} else {
				return m_sellOffers.count(mosaicId);
			}
		}

		void addOffer(model::OfferType type, const MosaicId& mosaicId, const model::OfferWithDuration* pOffer, const Height& deadline) {
			state::OfferBase baseOffer{pOffer->Mosaic.Amount, pOffer->Mosaic.Amount, pOffer->Cost, deadline, deadline, false};
			if (model::OfferType::Buy == type) {
				m_buyOffers.emplace(mosaicId, state::BuyOffer{baseOffer, pOffer->Cost});
			} else {
				m_sellOffers.emplace(mosaicId, state::SellOffer{baseOffer});
			}
		}

		void removeOffer(model::OfferType type, const MosaicId& mosaicId) {
			if (model::OfferType::Buy == type) {
				m_buyOffers.erase(mosaicId);
			} else {
				m_sellOffers.erase(mosaicId);
			}
		}

		state::OfferBase& getBaseOffer(model::OfferType type, const MosaicId& mosaicId) {
			return (model::OfferType::Buy == type) ?
			   dynamic_cast<state::OfferBase&>(m_buyOffers.at(mosaicId)) :
			   dynamic_cast<state::OfferBase&>(m_sellOffers.at(mosaicId));
		}

		bool empty() const {
			return m_buyOffers.empty() && m_sellOffers.empty();
		}

	public:
		/// Returns \c false at least one offer is expiring at \a height.
		bool isActive(catapult::Height height) const {
			return height < minExpiryHeight();
		}

	private:
		Key m_owner;
		SellOfferMap m_sellOffers;
		BuyOfferMap m_buyOffers;
	};
}}
