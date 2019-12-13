/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeEntry.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"
#include <cmath>

namespace catapult { namespace state {

	double OfferBase::price() const {
		if (catapult::Amount(0) == InitialAmount)
			CATAPULT_THROW_RUNTIME_ERROR("failed to calculate offer price, initial amount is zero");

		return static_cast<double>(InitialCost.unwrap()) / static_cast<double>(InitialAmount.unwrap());
	}

	catapult::Amount SellOffer::cost(const catapult::Amount& amount) const {
		if (catapult::Amount(0) == InitialAmount)
			CATAPULT_THROW_RUNTIME_ERROR("failed to calculate sell offer cost, initial amount is zero");

		auto cost = std::ceil(static_cast<double>(InitialCost.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialAmount.unwrap()));
		return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
	}

	SellOffer& SellOffer::operator+=(const model::MatchedOffer& offer) {
		if (model::OfferType::Sell != offer.Type)
			CATAPULT_THROW_INVALID_ARGUMENT("(add) matched offer type is not SELL");

		Amount = Amount + offer.Mosaic.Amount;
		return *this;
	}

	SellOffer& SellOffer::operator-=(const model::MatchedOffer& offer) {
		if (model::OfferType::Sell != offer.Type)
			CATAPULT_THROW_INVALID_ARGUMENT("(subtract) matched offer type is not SELL");

		if (offer.Mosaic.Amount > Amount)
			CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than sell offer amount", offer.Mosaic.Amount, Amount)

		Amount = Amount - offer.Mosaic.Amount;
		return *this;
	}

	catapult::Amount BuyOffer::cost(const catapult::Amount& amount) const {
		if (catapult::Amount(0) == InitialAmount)
			CATAPULT_THROW_RUNTIME_ERROR("failed to calculate buy offer cost, initial amount is zero");

		auto cost = std::floor(static_cast<double>(InitialCost.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialAmount.unwrap()));
		return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
	}

	BuyOffer& BuyOffer::operator+=(const model::MatchedOffer& offer) {
		if (model::OfferType::Buy != offer.Type)
			CATAPULT_THROW_INVALID_ARGUMENT("(add) matched offer type is not BUY");

		Amount = Amount + offer.Mosaic.Amount;
		ResidualCost = ResidualCost + offer.Cost;

		return *this;
	}

	BuyOffer& BuyOffer::operator-=(const model::MatchedOffer& offer) {
		if (model::OfferType::Buy != offer.Type)
			CATAPULT_THROW_INVALID_ARGUMENT("(subtract) matched offer type is not BUY");

		if (offer.Mosaic.Amount > Amount)
			CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than buy offer amount", offer.Mosaic.Amount, Amount)

		if (offer.Cost > ResidualCost)
			CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than residual cost", offer.Cost, ResidualCost)

		Amount = Amount - offer.Mosaic.Amount;
		ResidualCost = ResidualCost - offer.Cost;

		return *this;
	}

	/// Gets the height of the earliest expiring offer.
	Height ExchangeEntry::minExpiryHeight() const {
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
	Height ExchangeEntry::minPruneHeight() const {
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
	void ExchangeEntry::expireOffer(model::OfferType type, const MosaicId& mosaicId, const Height& height) {
		auto expireFunc = [height, mosaicId](auto& offers, auto& expiredOffers) {
			auto& offer = offers.at(mosaicId);
			auto &expiredOffersAtHeight = expiredOffers[height];
			if (expiredOffersAtHeight.count(mosaicId))
				CATAPULT_THROW_RUNTIME_ERROR_2("offer already expired at height", mosaicId, height);
			expiredOffersAtHeight.emplace(mosaicId, offer);
			offers.erase(mosaicId);
		};
		if (model::OfferType::Buy == type)
			expireFunc(m_buyOffers, m_expiredBuyOffers);
		else
			expireFunc(m_sellOffers, m_expiredSellOffers);
	}

	/// Moves offer of \a type with \a mosaicId back from expired offer buffer.
	void ExchangeEntry::unexpireOffer(model::OfferType type, const MosaicId& mosaicId, const Height& height) {
		auto unexpireFunc = [height, mosaicId](auto& offers, auto& expiredOffers) {
			if (offers.count(mosaicId))
				CATAPULT_THROW_RUNTIME_ERROR_1("offer exists", mosaicId);
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

	namespace {
		/// Moves offers to expired offer buffer if offer expiry height is equal \a height.
		template<typename TOfferMap, typename TExpiredOfferMap>
		void expireOffers(TOfferMap& offers, TExpiredOfferMap& expiredOffers, const Height& height, consumer<const typename TOfferMap::const_iterator&> action) {
			for (auto iter = offers.begin(); iter != offers.end();) {
				if (iter->second.Deadline == height) {
					auto &expiredOffersAtHeight = expiredOffers[height];
					if (expiredOffersAtHeight.count(iter->first))
						CATAPULT_THROW_RUNTIME_ERROR_2("offer already expired at height", iter->first, height);
					expiredOffersAtHeight.emplace(iter->first, iter->second);
					action(iter);
					iter = offers.erase(iter);
				} else {
					++iter;
				}
			}
		}

		/// Moves offers back from expired offer buffer if offer expiry height is equal \a height.
		template<typename TOfferMap, typename TExpiredOfferMap>
		void unexpireOffers(TOfferMap& offers, TExpiredOfferMap& expiredOffers, const Height& height, consumer<const typename TOfferMap::const_iterator&> action) {
			if (expiredOffers.count(height)) {
				auto& expiredOffersAtHeight = expiredOffers.at(height);
				for (auto iter = expiredOffersAtHeight.begin(); iter != expiredOffersAtHeight.end();) {
					if (offers.count(iter->first))
						CATAPULT_THROW_RUNTIME_ERROR_2("offer exists at height", iter->first, height);
					offers.emplace(iter->first, iter->second);
					action(offers.find(iter->first));
					iter = expiredOffersAtHeight.erase(iter);
				}
				expiredOffers.erase(height);
			}
		}
	}

	void ExchangeEntry::expireOffers(
			const Height& height,
			consumer<const BuyOfferMap::const_iterator&> buyOfferAction,
			consumer<const SellOfferMap::const_iterator&> sellOfferAction) {
		state::expireOffers(m_buyOffers, m_expiredBuyOffers, height, buyOfferAction);
		state::expireOffers(m_sellOffers, m_expiredSellOffers, height, sellOfferAction);
	}

	void ExchangeEntry::unexpireOffers(
			const Height& height,
			consumer<const BuyOfferMap::const_iterator&> buyOfferAction,
			consumer<const SellOfferMap::const_iterator&> sellOfferAction) {
		state::unexpireOffers(m_buyOffers, m_expiredBuyOffers, height, buyOfferAction);
		state::unexpireOffers(m_sellOffers, m_expiredSellOffers, height, sellOfferAction);
	}

	bool ExchangeEntry::offerExists(model::OfferType type, const MosaicId& mosaicId) const {
		if (model::OfferType::Buy == type)
			return m_buyOffers.count(mosaicId);
		else
			return m_sellOffers.count(mosaicId);
	}

	void ExchangeEntry::addOffer(const MosaicId& mosaicId, const model::OfferWithDuration* pOffer, const Height& deadline) {
		state::OfferBase baseOffer{pOffer->Mosaic.Amount, pOffer->Mosaic.Amount, pOffer->Cost, deadline};
		if (model::OfferType::Buy == pOffer->Type)
			m_buyOffers.emplace(mosaicId, state::BuyOffer{baseOffer, pOffer->Cost});
		else
			m_sellOffers.emplace(mosaicId, state::SellOffer{baseOffer});
	}

	void ExchangeEntry::removeOffer(model::OfferType type, const MosaicId& mosaicId) {
		if (model::OfferType::Buy == type)
			m_buyOffers.erase(mosaicId);
		else
			m_sellOffers.erase(mosaicId);
	}

	state::OfferBase& ExchangeEntry::getBaseOffer(model::OfferType type, const MosaicId& mosaicId) {
		bool isBuyOffer = (model::OfferType::Buy == type);
		if ((isBuyOffer && !m_buyOffers.count(mosaicId)) || (!isBuyOffer && !m_sellOffers.count(mosaicId)))
			CATAPULT_THROW_INVALID_ARGUMENT_2("offer doesn't exist", type, mosaicId)

		return (isBuyOffer) ?
		   dynamic_cast<state::OfferBase&>(m_buyOffers.at(mosaicId)) :
		   dynamic_cast<state::OfferBase&>(m_sellOffers.at(mosaicId));
	}

	bool ExchangeEntry::empty() const {
		return m_buyOffers.empty() && m_sellOffers.empty() && m_expiredBuyOffers.empty() && m_expiredSellOffers.empty();
	}
}}
