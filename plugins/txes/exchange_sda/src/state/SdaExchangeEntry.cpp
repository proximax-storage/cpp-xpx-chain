/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeEntry.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"
#include <cmath>

namespace catapult { namespace state {

	catapult::Amount SwapOffer::cost(const catapult::Amount& amount) const {
		if (catapult::Amount(0) == InitialMosaicGive)
			CATAPULT_THROW_RUNTIME_ERROR("failed to calculate swap offer cost, initial amount is zero");

		auto cost = std::ceil(static_cast<double>(InitialMosaicGet.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialMosaicGive.unwrap()));
		return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
	}

	SwapOffer& SwapOffer::operator+=(const model::MatchedSdaOffer& offer) {
		CurrentMosaicGive = CurrentMosaicGive + offer.MosaicGive.Amount;
		ResidualMosaicGet = ResidualMosaicGet + offer.MosaicGet.Amount;
		return *this;
	}

	SwapOffer& SwapOffer::operator-=(const model::MatchedSdaOffer& offer) {
		if (offer.MosaicGive.Amount > CurrentMosaicGive)
			CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than swap offer amount", offer.MosaicGive.Amount, CurrentMosaicGive)
		
		if (offer.MosaicGet.Amount > ResidualMosaicGet)
			CATAPULT_THROW_INVALID_ARGUMENT_2("subtracting value greater than residual cost", offer.MosaicGet.Amount, ResidualMosaicGet)

		CurrentMosaicGive = CurrentMosaicGive - offer.MosaicGive.Amount;
		ResidualMosaicGet = ResidualMosaicGet - offer.MosaicGet.Amount;
		return *this;
	}

	/// Gets the height of the earliest expiring offer.
	Height SdaExchangeEntry::minExpiryHeight() const {
		if (m_swapOffers.empty())
			return Invalid_Expiry_Height;

		Height expiryHeight = Height(std::numeric_limits<Height::ValueType>::max());
		std::for_each(m_swapOffers.begin(), m_swapOffers.end(), [&expiryHeight](const auto &pair) {
			if (expiryHeight > pair.second.Deadline)
				expiryHeight = pair.second.Deadline;
		});

		return expiryHeight;
	}

	/// Gets the earliest height at which to prune expiring offers.
	Height SdaExchangeEntry::minPruneHeight() const {
		return m_expiredSwapOffers.empty() ? Invalid_Expiry_Height : m_expiredSwapOffers.begin()->first;
	}

	/// Moves offer of \a type with \a mosaicId to expired offer buffer.
	void SdaExchangeEntry::expireOffer(const MosaicId& mosaicId, const Height& height) {
		auto expireFunc = [height, mosaicId](auto& offers, auto& expiredOffers) {
			auto& offer = offers.at(mosaicId);
			auto &expiredOffersAtHeight = expiredOffers[height];
			if (expiredOffersAtHeight.count(mosaicId))
				CATAPULT_THROW_RUNTIME_ERROR_2("offer already expired at height", mosaicId, height);
			expiredOffersAtHeight.emplace(mosaicId, offer);
			offers.erase(mosaicId);
		};
		
		expireFunc(m_swapOffers, m_expiredSwapOffers);
	}

	/// Moves offer of \a type with \a mosaicId back from expired offer buffer.
	void SdaExchangeEntry::unexpireOffer(const MosaicId& mosaicId, const Height& height) {
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
		
		unexpireFunc(m_swapOffers, m_expiredSwapOffers);
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
					if (iter->second.Deadline == height) {
						if (offers.count(iter->first))
							CATAPULT_THROW_RUNTIME_ERROR_2("offer exists at height", iter->first, height);
						offers.emplace(iter->first, iter->second);
						action(offers.find(iter->first));
						iter = expiredOffersAtHeight.erase(iter);
					} else {
						++iter;
					}
				}
				if (expiredOffersAtHeight.empty())
					expiredOffers.erase(height);
			}
		}
	}

	void SdaExchangeEntry::expireOffers(
			const Height& height,
			consumer<const SwapOfferMap::const_iterator&> swapOfferAction) {
		state::expireOffers(m_swapOffers, m_expiredSwapOffers, height, swapOfferAction);
	}

	void SdaExchangeEntry::unexpireOffers(
			const Height& height,
			consumer<const SwapOfferMap::const_iterator&> swapOfferAction) {
		state::unexpireOffers(m_swapOffers, m_expiredSwapOffers, height, swapOfferAction);
	}

	bool SdaExchangeEntry::offerExists(const MosaicId& mosaicId) const {
		return m_swapOffers.count(mosaicId);
	}

	void SdaExchangeEntry::addOffer(const MosaicId& mosaicId, const model::SdaOfferWithDuration* pOffer, const Height& deadline) {
		state::SdaOfferBase baseOffer{pOffer->MosaicGive.Amount, pOffer->MosaicGive.Amount, pOffer->MosaicGet.Amount, deadline};
		m_swapOffers.emplace(mosaicId, state::SwapOffer{baseOffer, pOffer->MosaicGet.Amount});
	}

	void SdaExchangeEntry::removeOffer(const MosaicId& mosaicId) {
		m_swapOffers.erase(mosaicId);
	}

	state::SdaOfferBase& SdaExchangeEntry::getSdaBaseOffer(const MosaicId& mosaicId) {
		if (!m_swapOffers.count(mosaicId))
			CATAPULT_THROW_INVALID_ARGUMENT_1("offer doesn't exist", mosaicId);

		return dynamic_cast<state::SdaOfferBase&>(m_swapOffers.at(mosaicId));
	}

	bool SdaExchangeEntry::empty() const {
		return m_swapOffers.empty() && m_expiredSwapOffers.empty();
	}
}}
