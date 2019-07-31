/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigBuilder.h"
#include "catapult/io/RawFile.h"

namespace catapult { namespace builders {

	CatapultConfigBuilder::CatapultConfigBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void CatapultConfigBuilder::setApplyHeightDelta(const BlockDuration& applyHeightDelta) {
		m_applyHeightDelta = applyHeightDelta;
	}

	void CatapultConfigBuilder::setBlockChainConfig(const std::string& file) {
		io::RawFile rawFile(file, io::OpenMode::Read_Only, io::LockMode::None);
		m_blockChainConfig.resize(rawFile.size());
		rawFile.read(MutableRawBuffer(m_blockChainConfig.data(), m_blockChainConfig.size()));
	}

	void CatapultConfigBuilder::setBlockChainConfig(const RawBuffer& blockChainConfig) {
		m_blockChainConfig.resize(blockChainConfig.Size);
		m_blockChainConfig.assign(blockChainConfig.pData, blockChainConfig.pData + blockChainConfig.Size);
	}

	void CatapultConfigBuilder::setSupportedVersionsConfig(const std::string& file) {
		io::RawFile rawFile(file, io::OpenMode::Read_Only, io::LockMode::None);
		m_supportedVersionsConfig.resize(rawFile.size());
		rawFile.read(MutableRawBuffer(m_supportedVersionsConfig.data(), m_supportedVersionsConfig.size()));
	}

	void CatapultConfigBuilder::setSupportedVersionsConfig(const RawBuffer& supportedVersionsConfig) {
		m_supportedVersionsConfig.resize(supportedVersionsConfig.Size);
		m_supportedVersionsConfig.assign(supportedVersionsConfig.pData, supportedVersionsConfig.pData + supportedVersionsConfig.Size);
	}

	template<typename TransactionType>
	std::unique_ptr<TransactionType> CatapultConfigBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType) + m_blockChainConfig.size() + m_supportedVersionsConfig.size());

		// 2. set fixed transaction fields
		pTransaction->ApplyHeightDelta = m_applyHeightDelta;
		pTransaction->BlockChainConfigSize = utils::checked_cast<size_t, uint32_t>(m_blockChainConfig.size());
		pTransaction->SupportedEntityVersionsSize = utils::checked_cast<size_t, uint32_t>(m_supportedVersionsConfig.size());

		// 3. set transaction attachments
		std::copy(m_blockChainConfig.cbegin(), m_blockChainConfig.cend(), pTransaction->BlockChainConfigPtr());
		std::copy(m_supportedVersionsConfig.cbegin(), m_supportedVersionsConfig.cend(), pTransaction->SupportedEntityVersionsPtr());

		return pTransaction;
	}

	std::unique_ptr<CatapultConfigBuilder::Transaction> CatapultConfigBuilder::build() const {
		return buildImpl<Transaction>();
	}

	std::unique_ptr<CatapultConfigBuilder::EmbeddedTransaction> CatapultConfigBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
