/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/exceptions.h"
#include "catapult/functions.h"
#include "src/model/SdaOffer.h"
#include <cmath>
#include <map>

namespace catapult { namespace state {

	struct SdaOfferBase {
	public:
		catapult::Amount CurrentMosaicGive;
		catapult::Amount InitialMosaicGive;
		catapult::Amount InitialMosaicGet;
		Height Deadline;
	};

	struct SwapOffer : public SdaOfferBase {
	public:
		catapult::Amount ResidualMosaicGet;

	public:
		catapult::Amount cost(const catapult::Amount& amount) const;
		SwapOffer& operator+=(const model::MatchedSdaOffer& offer);
		SwapOffer& operator-=(const model::MatchedSdaOffer& offer);
	};

	using SwapOfferMap = std::map<MosaicId, SwapOffer>;
	using ExpiredSwapOfferMap = std::map<Height, SwapOfferMap>;

	// SDA-SDA Exchange entry.
	class SdaExchangeEntry {
	public:
		// Creates an SDA-SDA exchange entry around \a owner and \a version.
		SdaExchangeEntry(const Key& owner, VersionType version = Current_Version)
			: m_owner(owner)
			, m_version(version)
		{}

	public:
		static constexpr Height Invalid_Expiry_Height = Height(0);
		static constexpr VersionType Current_Version = 1;

	public:
		/// Gets the entry version.
		VersionType version() const {
			return m_version;
		}

		/// Gets the owner of the offers.
		const Key& owner() const {
			return m_owner;
		}

		/// Gets the swap offers.
		SwapOfferMap& swapOffers() {
			return m_swapOffers;
		}

		/// Gets the swap offers.
		const SwapOfferMap& swapOffers() const {
			return m_swapOffers;
		}

	public:
		/// Gets the height of the earliest expiring offer.
		Height minExpiryHeight() const;

		/// Gets the earliest height at which to prune expiring offers.
		Height minPruneHeight() const;

		/// Moves offer of \a type with \a mosaicId to expired offer buffer.
		void expireOffer(const MosaicId& mosaicId, const Height& height);

		/// Moves offer of \a type with \a mosaicId back from expired offer buffer.
		void unexpireOffer(const MosaicId& mosaicId, const Height& height);

		void expireOffers(
			const Height& height,
			consumer<const SwapOfferMap::const_iterator&> swapOfferAction);

		void unexpireOffers(
			const Height& height,
			consumer<const SwapOfferMap::const_iterator&> swapOfferAction);

		bool offerExists(const MosaicId& mosaicId) const;
		void addOffer(const MosaicId& mosaicId, const model::MatchedSdaOffer* pOffer, const Height& deadline);
		void removeOffer(const MosaicId& mosaicId);
		state::SdaOfferBase& getSdaBaseOffer(const MosaicId& mosaicId);
		bool empty() const;

	public:
		/// Gets the expired swap offers.
		ExpiredSwapOfferMap& expiredSwapOffers() {
			return m_expiredSwapOffers;
		}

		/// Gets the expired sell offers.
		const ExpiredSwapOfferMap& expiredSwapOffers() const {
			return m_expiredSwapOffers;
		}

	private:
		Key m_owner;
		VersionType m_version;
		SwapOfferMap m_swapOffers;
		ExpiredSwapOfferMap m_expiredSwapOffers;
	};
}}
