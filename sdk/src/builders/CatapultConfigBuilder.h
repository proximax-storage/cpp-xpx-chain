/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/config/src/model/CatapultConfigTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class CatapultConfigBuilder : public TransactionBuilder {
	public:
		using Transaction = model::CatapultConfigTransaction;
		using EmbeddedTransaction = model::EmbeddedCatapultConfigTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		CatapultConfigBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a durationDelta of the contract.
		void setApplyHeightDelta(const BlockDuration& applyHeightDelta);

		/// Loads and sets the blockchain configuration from \a file.
		void setBlockChainConfig(const std::string& file);

		/// Sets the blockchain configuration to \a blockChainConfig.
		void setBlockChainConfig(const RawBuffer& blockChainConfig);

		/// Loads and sets the entity versions configuration from \a file.
		void setSupportedVersionsConfig(const std::string& file);

		/// Sets the supported the entity versions configuration to \a supportedVersionsConfig.
		void setSupportedVersionsConfig(const RawBuffer& supportedVersionsConfig);

	public:
		/// Builds a new modify multisig account transaction.
		std::unique_ptr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		std::unique_ptr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		std::unique_ptr<TTransaction> buildImpl() const;

	private:
		BlockDuration m_applyHeightDelta;
		std::vector<uint8_t> m_blockChainConfig;
		std::vector<uint8_t> m_supportedVersionsConfig;
	};
}}
