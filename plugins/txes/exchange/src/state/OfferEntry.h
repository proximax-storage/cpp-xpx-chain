/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/Offer.h"
#include <map>

namespace catapult { namespace state {

	using OfferMap = std::map<UnresolvedMosaicId, model::Offer>;

	// Offer entry.
	class OfferEntry {
	public:
		// Creates an exchange entry around \a transactionHash.
		OfferEntry(const utils::ShortHash& transactionHash, const Key& transactionSigner, model::OfferType offerType, const Height& deadline)
			: m_transactionHash(transactionHash)
			, m_transactionSigner(transactionSigner)
			, m_offerType(offerType)
			, m_deadline(deadline)
			, m_expiryHeight(deadline)
		{}

	public:

	public:
		/// Gets the hash of the transaction with offer.
		const utils::ShortHash& transactionHash() const {
			return m_transactionHash;
		}

		/// Gets the signer of the transaction with offer.
		const Key& transactionSigner() const {
			return m_transactionSigner;
		}

		/// Gets the offer type.
		model::OfferType offerType() const {
			return m_offerType;
		}

		/// Gets the offer deadline.
		const Height& deadline() const {
			return m_deadline;
		}

		/// Gets the height at which to remove the offer.
		const Height& expiryHeight() const {
			return m_expiryHeight;
		}

		/// Sets the offer \a expiryHeight.
		void setExpiryHeight(const Height& expiryHeight) {
			m_expiryHeight = expiryHeight;
		}

		/// Gets the offers.
		OfferMap& offers() {
			return m_offers;
		}

		/// Gets the offers.
		const OfferMap& offers() const {
			return m_offers;
		}

		/// Checks whether offers are fulfilled.
		bool fulfilled() {
			Amount sum(0);
			std::for_each(m_offers.begin(), m_offers.end(), [&sum](const auto& pair) {
				sum = Amount(sum.unwrap() + pair.second.Mosaic.Amount.unwrap());
			});

			return (Amount(0) == sum);
		}

		/// Gets the initial offers.
		OfferMap& initialOffers() {
			return m_initialOffers;
		}

		/// Gets the initial offers.
		const OfferMap& initialOffers() const {
			return m_initialOffers;
		}

	public:
		/// Returns \c true if offer is active at \a height.
		constexpr bool isActive(catapult::Height height) const {
			return height < m_expiryHeight;
		}

	private:
		utils::ShortHash m_transactionHash;
		Key m_transactionSigner;
		model::OfferType m_offerType;
		Height m_deadline;
		Height m_expiryHeight;
		OfferMap m_offers;
		OfferMap m_initialOffers;
	};
}}
