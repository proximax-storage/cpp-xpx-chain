/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/dbrb/src/model/AddDbrbProcessTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class AddDbrbProcessBuilder : public TransactionBuilder {
	public:
		using Transaction = model::AddDbrbProcessTransaction;
		using EmbeddedTransaction = model::EmbeddedAddDbrbProcessTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		AddDbrbProcessBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

		/// Adds \a harvesterKey to attached keys.
		void addHarvesterKey(const Key& harvesterKey);

	public:
		/// Builds a new modify multisig account transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		std::vector<Key> m_harvesterKeys;
	};
}}
