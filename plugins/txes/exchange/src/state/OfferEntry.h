/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeEntryUtils.h"

namespace catapult { namespace state {

	// Offer entry.
	class OfferEntry {
	public:
		// Creates an exchange entry around \a transactionHash.
		OfferEntry(const utils::ShortHash& transactionHash, const Key& transactionSigner)
			: m_transactionHash(transactionHash)
			, m_transactionSigner(transactionSigner)
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

		/// Gets the offers deadline.
		const Timestamp& deadline() const {
			return m_deadline;
		}

		/// Sets the offers \a deadline.
		void setDeadline(const Timestamp& deadline) {
			m_deadline = deadline;
		}

		/// Gets the deposit for the offers (zero for sell offers).
		const Amount& deposit() const {
			return m_deposit;
		}

		/// Sets the \a deposit.
		void setDeposit(const Amount& deposit) {
			m_deposit = deposit;
		}

		/// Decreases the \a deposit.
		void decreaseDeposit(const Amount& value) {
			m_deposit = Amount(m_deposit.unwrap() - value.unwrap());
		}

		/// Increases the \a deposit.
		void inreaseDeposit(const Amount& value) {
			m_deposit = Amount(m_deposit.unwrap() + value.unwrap());
		}

		/// Gets the offers.
		OfferMap& offers() {
			return m_offers;
		}

		/// Gets the offers.
		const OfferMap& offers() const {
			return m_offers;
		}

		/// Removes fulfilled offers.
		void cleanupOffers() {
			for (auto iter = m_offers.begin(); iter != m_offers.end();) {
				iter = (iter->second.Mosaic.Amount == Amount(0)) ? m_offers.erase(iter) : ++iter;
			}
		}

	private:
		utils::ShortHash m_transactionHash;
		Key m_transactionSigner;
		Timestamp m_deadline;
		Amount m_deposit;
		OfferMap m_offers;
	};
}}
