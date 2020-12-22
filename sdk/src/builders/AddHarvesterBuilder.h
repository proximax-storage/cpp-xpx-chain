/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/committee/src/model/AddHarvesterTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class AddHarvesterBuilder : public TransactionBuilder {
	public:
		using Transaction = model::AddHarvesterTransaction;
		using EmbeddedTransaction = model::EmbeddedAddHarvesterTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		AddHarvesterBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Builds a new modify multisig account transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;
	};
}}
