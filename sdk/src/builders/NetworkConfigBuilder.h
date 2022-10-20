/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionBuilder.h"
#include "plugins/txes/config/src/model/NetworkConfigTransaction.h"
#include "catapult/io/RawFile.h"
#include <vector>

namespace catapult { namespace builders {

	namespace detail{
		template<typename TTransaction>
		struct NetworkConfigEmbeddedDeducer;
		template<>
		struct NetworkConfigEmbeddedDeducer<model::NetworkConfigTransaction>{
			using Transaction = model::NetworkConfigTransaction;
			using EmbeddedTransaction = model::EmbeddedNetworkConfigTransaction;
		};
		template<>
		struct NetworkConfigEmbeddedDeducer<model::NetworkConfigAbsoluteHeightTransaction>{
			using Transaction = model::NetworkConfigAbsoluteHeightTransaction;
			using EmbeddedTransaction = model::EmbeddedNetworkConfigAbsoluteHeightTransaction;
		};
	}

	/// Builder for a catapult upgrade transaction.
	template<typename TTransaction, typename TEmbeddedTransaction = typename detail::NetworkConfigEmbeddedDeducer<TTransaction>::EmbeddedTransaction>
	class NetworkConfigBuilder : public TransactionBuilder {
	public:
		using TransactionType = TTransaction;
		using EmbeddedTransactionType = TEmbeddedTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		NetworkConfigBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer),
			m_applyHeightDelta(0)
		{}

	public:
		/// Sets the \a durationDelta of the contract.
		void setApplyHeightDelta(const BlockDuration& applyHeightDelta) {
			m_applyHeightDelta = applyHeightDelta.unwrap();
		}

		/// Sets the \a durationDelta of the contract.
		void setApplyHeight(const Height& applyHeight) {
			m_applyHeightDelta = applyHeight.unwrap();
		}

		/// Loads and sets the blockchain configuration from \a file.
		void setBlockChainConfig(const std::string& file) {
			io::RawFile rawFile(file, io::OpenMode::Read_Only, io::LockMode::None);
			m_networkConfig.resize(rawFile.size());
			rawFile.read(MutableRawBuffer(m_networkConfig.data(), m_networkConfig.size()));
		}

		/// Sets the blockchain configuration to \a networkConfig.
		void setBlockChainConfig(const RawBuffer& networkConfig) {
			m_networkConfig.resize(networkConfig.Size);
			m_networkConfig.assign(networkConfig.pData, networkConfig.pData + networkConfig.Size);
		}

		/// Loads and sets the entity versions configuration from \a file.
		void setSupportedVersionsConfig(const std::string& file) {
			io::RawFile rawFile(file, io::OpenMode::Read_Only, io::LockMode::None);
			m_supportedVersionsConfig.resize(rawFile.size());
			rawFile.read(MutableRawBuffer(m_supportedVersionsConfig.data(), m_supportedVersionsConfig.size()));
		}

		/// Sets the supported the entity versions configuration to \a supportedVersionsConfig.
		void setSupportedVersionsConfig(const RawBuffer& supportedVersionsConfig) {
			m_supportedVersionsConfig.resize(supportedVersionsConfig.Size);
			m_supportedVersionsConfig.assign(supportedVersionsConfig.pData, supportedVersionsConfig.pData + supportedVersionsConfig.Size);
		}
	public:
		/// Builds a new modify multisig account transaction.
		model::UniqueEntityPtr<TransactionType> build() const {
			return buildImpl<TransactionType>();
		}

		/// Builds a new embedded modify multisig account transaction.
		model::UniqueEntityPtr<EmbeddedTransactionType> buildEmbedded() const {
			return buildImpl<EmbeddedTransactionType>();
		}

	private:
		template<typename TTransactionType>
		model::UniqueEntityPtr<TTransactionType> buildImpl() const{
			// 1. allocate, zero (header), set model::Transaction fields
			auto pTransaction = createTransaction<TTransactionType>(sizeof(TTransactionType) + m_networkConfig.size() + m_supportedVersionsConfig.size());

			// 2. set fixed transaction fields
			if constexpr(std::is_same_v<TransactionType, model::NetworkConfigTransaction>)
				pTransaction->ApplyHeightDelta = BlockDuration(m_applyHeightDelta);
			else
				pTransaction->ApplyHeight = Height(m_applyHeightDelta);
			pTransaction->BlockChainConfigSize = utils::checked_cast<size_t, uint32_t>(m_networkConfig.size());
			pTransaction->SupportedEntityVersionsSize = utils::checked_cast<size_t, uint32_t>(m_supportedVersionsConfig.size());

			// 3. set transaction attachments
			std::copy(m_networkConfig.cbegin(), m_networkConfig.cend(), pTransaction->BlockChainConfigPtr());
			std::copy(m_supportedVersionsConfig.cbegin(), m_supportedVersionsConfig.cend(), pTransaction->SupportedEntityVersionsPtr());

			return pTransaction;
		}

	private:
		uint64_t m_applyHeightDelta;
		std::vector<uint8_t> m_networkConfig;
		std::vector<uint8_t> m_supportedVersionsConfig;
	};
}}
