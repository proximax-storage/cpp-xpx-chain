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

	struct SdaOfferBalance {
	public:
		catapult::Amount CurrentMosaicGive;
		catapult::Amount CurrentMosaicGet;
		catapult::Amount InitialMosaicGive;
		catapult::Amount InitialMosaicGet;
		Height Deadline;

	public:
		catapult::Amount cost(const catapult::Amount& amount) const;
		SdaOfferBalance& sender(const model::SdaOfferWithOwnerAndDuration& offer);
		SdaOfferBalance& receiver(const model::SdaOfferWithOwnerAndDuration& offer);
	};

	SdaOfferBalance& operator+=(const model::SdaOfferWithOwnerAndDuration& offer, const model::SdaOfferWithOwnerAndDuration& poffer);
	using SdaOfferBalanceMap = std::map<MosaicId, SdaOfferBalance>;
	using ExpiredSdaOfferBalanceMap = std::map<Height, SdaOfferBalanceMap>;

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

		/// Gets the balance SDA-SDA offers.
		SdaOfferBalanceMap& sdaOfferBalances() {
			return m_sdaOfferBalances;
		}

		/// Gets the balance SDA-SDA offers.
		const SdaOfferBalanceMap& sdaOfferBalances() const {
			return m_sdaOfferBalances;
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
			consumer<const SdaOfferBalanceMap::const_iterator&> sdaOfferBalanceAction);

		void unexpireOffers(
			const Height& height,
			consumer<const SdaOfferBalanceMap::const_iterator&> sdaOfferBalanceAction);

		bool offerExists(const MosaicId& mosaicId) const;
		void addOffer(const MosaicId& mosaicId, const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline);
		void removeOffer(const MosaicId& mosaicId);
		state::SdaOfferBalance& getSdaOfferBalance(const MosaicId& mosaicId);
		bool empty() const;

	public:
		/// Gets the expired swap offers.
		ExpiredSdaOfferBalanceMap& expiredSdaOfferBalances() {
			return m_expiredSdaOfferBalances;
		}

		/// Gets the expired sell offers.
		const ExpiredSdaOfferBalanceMap& expiredSdaOfferBalances() const {
			return m_expiredSdaOfferBalances;
		}

	private:
		Key m_owner;
		VersionType m_version;
		SdaOfferBalanceMap m_sdaOfferBalances;
		ExpiredSdaOfferBalanceMap m_expiredSdaOfferBalances;
	};
}}
