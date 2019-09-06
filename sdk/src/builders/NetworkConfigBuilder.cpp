/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigBuilder.h"
#include "catapult/io/RawFile.h"

namespace catapult { namespace builders {

	NetworkConfigBuilder::NetworkConfigBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void NetworkConfigBuilder::setApplyHeightDelta(const BlockDuration& applyHeightDelta) {
		m_applyHeightDelta = applyHeightDelta;
	}

	void NetworkConfigBuilder::setBlockChainConfig(const std::string& file) {
		io::RawFile rawFile(file, io::OpenMode::Read_Only, io::LockMode::None);
		m_networkConfig.resize(rawFile.size());
		rawFile.read(MutableRawBuffer(m_networkConfig.data(), m_networkConfig.size()));
	}

	void NetworkConfigBuilder::setBlockChainConfig(const RawBuffer& networkConfig) {
		m_networkConfig.resize(networkConfig.Size);
		m_networkConfig.assign(networkConfig.pData, networkConfig.pData + networkConfig.Size);
	}

	void NetworkConfigBuilder::setSupportedVersionsConfig(const std::string& file) {
		io::RawFile rawFile(file, io::OpenMode::Read_Only, io::LockMode::None);
		m_supportedVersionsConfig.resize(rawFile.size());
		rawFile.read(MutableRawBuffer(m_supportedVersionsConfig.data(), m_supportedVersionsConfig.size()));
	}

	void NetworkConfigBuilder::setSupportedVersionsConfig(const RawBuffer& supportedVersionsConfig) {
		m_supportedVersionsConfig.resize(supportedVersionsConfig.Size);
		m_supportedVersionsConfig.assign(supportedVersionsConfig.pData, supportedVersionsConfig.pData + supportedVersionsConfig.Size);
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> NetworkConfigBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType) + m_networkConfig.size() + m_supportedVersionsConfig.size());

		// 2. set fixed transaction fields
		pTransaction->ApplyHeightDelta = m_applyHeightDelta;
		pTransaction->BlockChainConfigSize = utils::checked_cast<size_t, uint32_t>(m_networkConfig.size());
		pTransaction->SupportedEntityVersionsSize = utils::checked_cast<size_t, uint32_t>(m_supportedVersionsConfig.size());

		// 3. set transaction attachments
		std::copy(m_networkConfig.cbegin(), m_networkConfig.cend(), pTransaction->BlockChainConfigPtr());
		std::copy(m_supportedVersionsConfig.cbegin(), m_supportedVersionsConfig.cend(), pTransaction->SupportedEntityVersionsPtr());

		return pTransaction;
	}

	model::UniqueEntityPtr<NetworkConfigBuilder::Transaction> NetworkConfigBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<NetworkConfigBuilder::EmbeddedTransaction> NetworkConfigBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
