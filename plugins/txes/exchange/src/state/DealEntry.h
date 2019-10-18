/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeEntryUtils.h"

namespace catapult { namespace state {

	// Deal entry.
	class DealEntry {
	public:
		// Creates an exchange entry around \a transactionHash.
		DealEntry(const utils::ShortHash& transactionHash) : m_transactionHash(transactionHash)
		{}

	public:
		using AcceptedDeals = std::map<utils::ShortHash, OfferMap>;
		using DepositMap = std::map<utils::ShortHash, Amount>;

	public:
		/// Gets the hash of the transaction with suggested offer.
		const utils::ShortHash& transactionHash() const {
			return m_transactionHash;
		}

		/// Gets the deposit for the offers (zero for sell offers).
		const Amount& deposit() const {
			return m_deposit;
		}

		/// Sets the \a deposit.
		void setDeposit(const Amount& deposit) {
			m_deposit = deposit;
		}

		/// Gets the accepted offer deposits.
		DepositMap& deposits() {
			return m_deposits;
		}

		/// Gets the accepted offer deposits.
		const DepositMap& deposits() const {
			return m_deposits;
		}

		/// Gets the suggested deals.
		OfferMap& suggestedDeals() {
			return m_suggestedDeals;
		}

		/// Gets the suggested deals.
		const OfferMap& suggestedDeals() const {
			return m_suggestedDeals;
		}

		/// Gets the accepted deals.
		AcceptedDeals& acceptedDeals() {
			return m_acceptedDeals;
		}

		/// Gets the accepted deals.
		const AcceptedDeals& acceptedDeals() const {
			return m_acceptedDeals;
		}

	private:
		utils::ShortHash m_transactionHash;
		Amount m_deposit;
		DepositMap m_deposits;
		OfferMap m_suggestedDeals;
		AcceptedDeals m_acceptedDeals;
	};
}}
