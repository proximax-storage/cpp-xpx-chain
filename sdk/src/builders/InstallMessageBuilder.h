///**
//*** Copyright 2023 ProximaX Limited. All rights reserved.
//*** Use of this source code is governed by the Apache 2.0
//*** license that can be found in the LICENSE file.
//**/
//
//#pragma once
//#include "TransactionBuilder.h"
//#include "plugins/txes/dbrb/src/model/InstallMessageTransaction.h"
//#include "catapult/dbrb/Messages.h"
//#include <vector>
//
//namespace catapult { namespace builders {
//
//	/// Builder for an install message transaction.
//	class InstallMessageBuilder : public TransactionBuilder {
//	public:
//		using Transaction = model::InstallMessageTransaction;
//		using EmbeddedTransaction = model::EmbeddedInstallMessageTransaction;
//
//		/// Creates a builder for building an install message transaction from \a signer
//		/// for the network specified by \a networkIdentifier.
//		InstallMessageBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer, dbrb::InstallMessage& message);
//
//	public:
//		/// Builds a new install message transaction.
//		model::UniqueEntityPtr<Transaction> build() const;
//
//		/// Builds a new embedded install message transaction.
//		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;
//
//	private:
//		template<typename TTransaction>
//		model::UniqueEntityPtr<TTransaction> buildImpl() const;
//
//	private:
//		dbrb::InstallMessage& m_message;
//	};
//}}
