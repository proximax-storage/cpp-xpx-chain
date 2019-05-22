/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/upgrade/src/model/CatapultUpgradeTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class CatapultUpgradeBuilder : public TransactionBuilder {
	public:
		using Transaction = model::CatapultUpgradeTransaction;
		using EmbeddedTransaction = model::EmbeddedCatapultUpgradeTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		CatapultUpgradeBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a durationDelta of the contract.
		void setUpgradePeriod(const uint16_t& upgradePeriod);

		/// Sets the \a hash of an entity passed from customers to executors (e.g. file hash).
		void setNewCatapultVersion(const uint64_t& newCatapultVersion);

	public:
		/// Builds a new modify multisig account transaction.
		std::unique_ptr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		std::unique_ptr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		std::unique_ptr<TTransaction> buildImpl() const;

	private:
		uint16_t m_upgradePeriod;
		uint64_t m_newCatapultVersion;
	};
}}
