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

	catapult::Amount SdaOfferBalance::cost(const catapult::Amount& amount) const {
		if (catapult::Amount(0) == InitialMosaicGive)
			CATAPULT_THROW_RUNTIME_ERROR("failed to calculate offer cost, initial amount is zero");

		auto cost = std::ceil(static_cast<double>(InitialMosaicGet.unwrap()) * static_cast<double>(amount.unwrap()) / static_cast<double>(InitialMosaicGive.unwrap()));
		return catapult::Amount(static_cast<typeof(Amount::ValueType)>(cost));
	}

	SdaOfferBalance& SdaOfferBalance::operator+=(const model::SdaOfferWithOwnerAndDuration& offer) {
		CurrentMosaicGet = CurrentMosaicGet + offer.MosaicGet.Amount;
		return *this;
	}

	SdaOfferBalance& SdaOfferBalance::operator-=(const model::SdaOfferWithOwnerAndDuration& offer) {
		if (offer.MosaicGive.Amount > CurrentMosaicGive) {
			CurrentMosaicGive = offer.MosaicGive.Amount - CurrentMosaicGive;
		}
		else {
			CurrentMosaicGive = CurrentMosaicGive - offer.MosaicGive.Amount;
		}
		return *this;
	}

	/// Gets the height of the earliest expiring offer.
	Height SdaExchangeEntry::minExpiryHeight() const {
		if (m_sdaOfferBalances.empty())
			return Invalid_Expiry_Height;

		Height expiryHeight = Height(std::numeric_limits<Height::ValueType>::max());
		std::for_each(m_sdaOfferBalances.begin(), m_sdaOfferBalances.end(), [&expiryHeight](const auto &pair) {
			if (expiryHeight > pair.second.Deadline)
				expiryHeight = pair.second.Deadline;
		});

		return expiryHeight;
	}

	/// Gets the earliest height at which to prune expiring offers.
	Height SdaExchangeEntry::minPruneHeight() const {
		return m_expiredSdaOfferBalances.empty() ? Invalid_Expiry_Height : m_expiredSdaOfferBalances.begin()->first;
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
		
		expireFunc(m_sdaOfferBalances, m_expiredSdaOfferBalances);
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
		
		unexpireFunc(m_sdaOfferBalances, m_expiredSdaOfferBalances);
	}

	/// Moves offers to expired offer buffer if offer expiry height is equal \a height.
	void SdaExchangeEntry::expireOffers(const Height& height, consumer<const SdaOfferBalanceMap::const_iterator&> sdaOfferBalanceAction) {
		for (auto iter = m_sdaOfferBalances.begin(); iter != m_sdaOfferBalances.end();) {
			if (iter->second.Deadline == height) {
				auto &expiredOffersAtHeight = m_expiredSdaOfferBalances[height];
				if (expiredOffersAtHeight.count(iter->first))
					CATAPULT_THROW_RUNTIME_ERROR_2("offer already expired at height", iter->first, height);
				expiredOffersAtHeight.emplace(iter->first, iter->second);
				sdaOfferBalanceAction(iter);
				iter = m_sdaOfferBalances.erase(iter);
			} else {
				++iter;
			}
		}
	}

	/// Moves offers back from expired offer buffer if offer expiry height is equal \a height.
	void SdaExchangeEntry::unexpireOffers(const Height& height, consumer<const SdaOfferBalanceMap::const_iterator&> sdaOfferBalanceAction) {
		if (m_expiredSdaOfferBalances.count(height)) {
			auto& expiredOffersAtHeight = m_expiredSdaOfferBalances.at(height);
			for (auto iter = expiredOffersAtHeight.begin(); iter != expiredOffersAtHeight.end();) {
				if (iter->second.Deadline == height) {
					if (m_sdaOfferBalances.count(iter->first))
						CATAPULT_THROW_RUNTIME_ERROR_2("offer exists at height", iter->first, height);
					m_sdaOfferBalances.emplace(iter->first, iter->second);
					sdaOfferBalanceAction(m_sdaOfferBalances.find(iter->first));
					iter = expiredOffersAtHeight.erase(iter);
				} else {
					++iter;
				}
			}
			if (expiredOffersAtHeight.empty())
				m_expiredSdaOfferBalances.erase(height);
		}
	}

	bool SdaExchangeEntry::offerExists(const MosaicId& mosaicId) const {
		return m_sdaOfferBalances.count(mosaicId);
	}

	void SdaExchangeEntry::addOffer(const MosaicId& mosaicId, const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline) {
		state::SdaOfferBalance baseOffer{pOffer->MosaicGive.Amount, pOffer->MosaicGet.Amount, pOffer->MosaicGive.Amount, pOffer->MosaicGet.Amount, deadline};
		m_sdaOfferBalances.emplace(mosaicId, baseOffer);
	}

	void SdaExchangeEntry::removeOffer(const MosaicId& mosaicId) {
		m_sdaOfferBalances.erase(mosaicId);
	}

	state::SdaOfferBalance& SdaExchangeEntry::getSdaOfferBalance(const MosaicId& mosaicId) {
		if (!m_sdaOfferBalances.count(mosaicId))
			CATAPULT_THROW_INVALID_ARGUMENT_1("offer doesn't exist", mosaicId);

		return dynamic_cast<state::SdaOfferBalance&>(m_sdaOfferBalances.at(mosaicId));
	}

	bool SdaExchangeEntry::empty() const {
		return m_sdaOfferBalances.empty() && m_expiredSdaOfferBalances.empty();
	}
}}
