/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/lock_fund/src/model/LockFundCancelUnlockTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a transfer transaction.
	class LockFundCancelUnlockBuilder : public TransactionBuilder {
	public:
		using Transaction = model::LockFundCancelUnlockTransaction;
		using EmbeddedTransaction = model::EmbeddedLockFundCancelUnlockTransaction;

	public:
		/// Creates a transfer builder for building a transfer transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		LockFundCancelUnlockBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a targetHeight.
		void setHeight(Height targetHeight);
	public:
		/// Builds a new transfer transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded transfer transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:

		Height m_targetHeight;
	};
}}
