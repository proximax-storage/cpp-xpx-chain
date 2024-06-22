/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/dbrb/src/model/AddOrUpdateDbrbProcessTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for an add or update DBRB process transaction.
	class AddOrUpdateDbrbProcessBuilder : public TransactionBuilder {
	public:
		using Transaction = model::AddOrUpdateDbrbProcessTransaction;
		using EmbeddedTransaction = model::EmbeddedAddOrUpdateDbrbProcessTransaction;

		/// Creates an add or update DBRB process builder for building an add or update DBRB process transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		AddOrUpdateDbrbProcessBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

		/// Adds \a harvesterKey to attached keys.
		void addHarvesterKey(const Key& harvesterKey);

		/// Set software version.
		void setBlockchainVersion(const BlockchainVersion& blockchainVersion);

	public:
		/// Builds a new add or update DBRB process transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded add or update DBRB process transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		std::vector<Key> m_harvesterKeys;
		BlockchainVersion m_blockchainVersion;
	};
}}
