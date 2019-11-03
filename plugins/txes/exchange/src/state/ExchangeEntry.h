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

		/// Gets the height of the earliest expiring offer.
		Height minExpiryHeight() const {
			if (m_buyOffers.empty() && m_sellOffers.empty())
				return Invalid_Expiry_Height;

			Height expiryHeight = Height(std::numeric_limits<Height::ValueType>::max());
			std::for_each(m_sellOffers.begin(), m_sellOffers.end(), [&expiryHeight](const auto &pair) {
				if (expiryHeight > pair.second.Deadline)
					expiryHeight = pair.second.Deadline;
			});
			std::for_each(m_buyOffers.begin(), m_buyOffers.end(), [&expiryHeight](const auto &pair) {
				if (expiryHeight > pair.second.Deadline)
					expiryHeight = pair.second.Deadline;
			});

			return expiryHeight;
		}

		/// Gets the earliest height at which to prune expiring offers.
		Height minPruneHeight() const {
			uint8_t expiredBuyOffersFlag = m_expiredBuyOffers.empty() ? 0 : 1;
			uint8_t expiredSellOffersFlag = m_expiredSellOffers.empty() ? 0 : 2;
			switch (expiredBuyOffersFlag | expiredSellOffersFlag) {
				case 0:
					return Invalid_Expiry_Height;
				case 1:
					return m_expiredBuyOffers.begin()->first;
				case 2:
					return m_expiredSellOffers.begin()->first;
				default:
					return std::min(m_expiredBuyOffers.begin()->first, m_expiredSellOffers.begin()->first);
			}
		}

		/// Moves offer of \a type with \a mosaicId to expired offer buffer.
		void expireOffer(model::OfferType type, const MosaicId& mosaicId, const Height& height) {
			auto expireFunc = [height, mosaicId](auto& offers, auto& expiredOffers) {
				auto& offer = offers.at(mosaicId);
				auto &expiredOffersAtHeight = expiredOffers[height];
				if (expiredOffersAtHeight.count(mosaicId))
					CATAPULT_THROW_RUNTIME_ERROR_2("offer with mosaic already expired at height", mosaicId, height);
				expiredOffersAtHeight.emplace(mosaicId, offer);
				offers.erase(mosaicId);
			};
			if (model::OfferType::Buy == type)
				expireFunc(m_buyOffers, m_expiredBuyOffers);
			else
				expireFunc(m_sellOffers, m_expiredSellOffers);
		}

		/// Moves offer of \a type with \a mosaicId back from expired offer buffer.
		void unexpireOffer(model::OfferType type, const MosaicId& mosaicId, const Height& height) {
			auto unexpireFunc = [height, mosaicId](auto& offers, auto& expiredOffers) {
				if (offers.count(mosaicId))
					CATAPULT_THROW_RUNTIME_ERROR_1("offer with mosaic id exists", mosaicId);
				auto& expiredOffersAtHeight = expiredOffers.at(height);
				auto& offer = expiredOffersAtHeight.at(mosaicId);
				offers.emplace(mosaicId, offer);
				expiredOffersAtHeight.erase(mosaicId);
				if (expiredOffersAtHeight.empty())
					expiredOffers.erase(height);
			};
			if (model::OfferType::Buy == type)
				unexpireFunc(m_buyOffers, m_expiredBuyOffers);
			else
				unexpireFunc(m_sellOffers, m_expiredSellOffers);
		}

		/// Moves offers to expired offer buffer if offer expiry height is equal \a height.
		void expireOffers(const Height& height) {
			auto expireFunc = [height](auto& offers, auto& expiredOffers) {
				for (auto iter = offers.begin(); iter != offers.end();) {
					if (iter->second.Deadline == height) {
						auto &expiredOffersAtHeight = expiredOffers[height];
						if (expiredOffersAtHeight.count(iter->first))
							CATAPULT_THROW_RUNTIME_ERROR_2("offer with mosaic id already expired at height", iter->first, height);
						expiredOffersAtHeight.emplace(iter->first, iter->second);
						iter = offers.erase(iter);
					} else {
						++iter;
					}
				}
			};
			expireFunc(m_buyOffers, m_expiredBuyOffers);
			expireFunc(m_sellOffers, m_expiredSellOffers);
		}

		/// Moves offers back from expired offer buffer if offer expiry height is equal \a height.
		void unexpireOffers(const Height& height) {
			auto unexpireFunc = [height](auto& offers, auto& expiredOffers) {
				if (expiredOffers.count(height)) {
					auto& expiredOffersAtHeight = expiredOffers.at(height);
					for (auto iter = expiredOffersAtHeight.begin(); iter != expiredOffersAtHeight.end();) {
						if (offers.count(iter->first))
							CATAPULT_THROW_RUNTIME_ERROR_2("offer with mosaic id exists at height", iter->first, height);
						offers.emplace(iter->first, iter->second);
						iter = expiredOffersAtHeight.erase(iter);
					}
				}
			};
			unexpireFunc(m_buyOffers, m_expiredBuyOffers);
			unexpireFunc(m_sellOffers, m_expiredSellOffers);
		}

		bool offerExists(model::OfferType type, const MosaicId& mosaicId) const {
			if (model::OfferType::Buy == type)
				return m_buyOffers.count(mosaicId);
			else
				return m_sellOffers.count(mosaicId);
		}

		void addOffer(model::OfferType type, const MosaicId& mosaicId, const model::OfferWithDuration* pOffer, const Height& deadline) {
			state::OfferBase baseOffer{pOffer->Mosaic.Amount, pOffer->Mosaic.Amount, pOffer->Cost, deadline};
			if (model::OfferType::Buy == type)
				m_buyOffers.emplace(mosaicId, state::BuyOffer{baseOffer, pOffer->Cost});
			else
				m_sellOffers.emplace(mosaicId, state::SellOffer{baseOffer});
		}

		void removeOffer(model::OfferType type, const MosaicId& mosaicId) {
			if (model::OfferType::Buy == type)
				m_buyOffers.erase(mosaicId);
			else
				m_sellOffers.erase(mosaicId);
		}

		state::OfferBase& getBaseOffer(model::OfferType type, const MosaicId& mosaicId) {
			return (model::OfferType::Buy == type) ?
			   dynamic_cast<state::OfferBase&>(m_buyOffers.at(mosaicId)) :
			   dynamic_cast<state::OfferBase&>(m_sellOffers.at(mosaicId));
		}

		bool empty() const {
			return m_buyOffers.empty() && m_sellOffers.empty() && m_expiredBuyOffers.empty() && m_expiredSellOffers.empty();
		}

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
