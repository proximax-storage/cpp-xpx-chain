/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/lock_fund/src/model/LockFundTransferTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a transfer transaction.
	class LockFundTransferBuilder : public TransactionBuilder {
	public:
		using Transaction = model::LockFundTransferTransaction;
		using EmbeddedTransaction = model::EmbeddedLockFundTransferTransaction;

	public:
		/// Creates a transfer builder for building a transfer transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		LockFundTransferBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a action to perform, lock or unlock.
		void setAction(model::LockFundAction action);
		/// Sets the \a duration until unlock.
		void setDuration(BlockDuration duration);
		/// Adds \a mosaic to attached mosaics.
		void addMosaic(const model::UnresolvedMosaic& mosaic);

	public:
		/// Builds a new transfer transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded transfer transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:

		BlockDuration m_duration;
		model::LockFundAction m_action;
		std::vector<model::UnresolvedMosaic> m_mosaics;
	};
}}
