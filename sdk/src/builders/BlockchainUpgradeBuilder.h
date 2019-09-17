/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/upgrade/src/model/BlockchainUpgradeTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class BlockchainUpgradeBuilder : public TransactionBuilder {
	public:
		using Transaction = model::BlockchainUpgradeTransaction;
		using EmbeddedTransaction = model::EmbeddedBlockchainUpgradeTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		BlockchainUpgradeBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a durationDelta of the contract.
		void setUpgradePeriod(const BlockDuration& upgradePeriod);

		/// Sets the \a hash of an entity passed from customers to executors (e.g. file hash).
		void setNewBlockchainVersion(const BlockchainVersion& newBlockchainVersion);

	public:
		/// Builds a new modify multisig account transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		BlockDuration m_upgradePeriod;
		BlockchainVersion m_newBlockchainVersion;
	};
}}
