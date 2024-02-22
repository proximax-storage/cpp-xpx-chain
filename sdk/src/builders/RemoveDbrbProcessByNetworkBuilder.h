/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/dbrb/src/model/RemoveDbrbProcessByNetworkTransaction.h"
#include "catapult/model/Cosignature.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class RemoveDbrbProcessByNetworkBuilder : public TransactionBuilder {
	public:
		using Transaction = model::RemoveDbrbProcessByNetworkTransaction;
		using EmbeddedTransaction = model::EmbeddedRemoveDbrbProcessByNetworkTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		RemoveDbrbProcessByNetworkBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

		// Sets the process ID to remove from DBRB system.
		void setProcessId(const dbrb::ProcessId& id);

		// Sets the start time of collecting votes to remove the process from DBRB system.
		void setTimestamp(const Timestamp& timestamp);

		/// Adds \a vote to attached cosignatures.
		void addVote(model::Cosignature vote);

	public:
		/// Builds a new modify multisig account transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		dbrb::ProcessId m_processId;
		catapult::Timestamp m_timestamp;
		std::vector<model::Cosignature> m_votes;
	};
}}
