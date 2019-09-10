/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/config/src/model/NetworkConfigTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class NetworkConfigBuilder : public TransactionBuilder {
	public:
		using Transaction = model::NetworkConfigTransaction;
		using EmbeddedTransaction = model::EmbeddedNetworkConfigTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		NetworkConfigBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a durationDelta of the contract.
		void setApplyHeightDelta(const BlockDuration& applyHeightDelta);

		/// Loads and sets the blockchain configuration from \a file.
		void setBlockChainConfig(const std::string& file);

		/// Sets the blockchain configuration to \a networkConfig.
		void setBlockChainConfig(const RawBuffer& networkConfig);

		/// Loads and sets the entity versions configuration from \a file.
		void setSupportedVersionsConfig(const std::string& file);

		/// Sets the supported the entity versions configuration to \a supportedVersionsConfig.
		void setSupportedVersionsConfig(const RawBuffer& supportedVersionsConfig);

	public:
		/// Builds a new modify multisig account transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		BlockDuration m_applyHeightDelta;
		std::vector<uint8_t> m_networkConfig;
		std::vector<uint8_t> m_supportedVersionsConfig;
	};
}}
