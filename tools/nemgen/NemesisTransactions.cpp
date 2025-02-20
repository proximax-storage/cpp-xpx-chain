/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "NemesisTransactions.h"
#include "catapult/builders/AddHarvesterBuilder.h"
#include "catapult/builders/NetworkConfigBuilder.h"
#include "catapult/builders/BlockchainUpgradeBuilder.h"
#include "catapult/builders/MosaicAliasBuilder.h"
#include "catapult/builders/MosaicDefinitionBuilder.h"
#include "catapult/builders/MosaicSupplyChangeBuilder.h"
#include "catapult/builders/RegisterNamespaceBuilder.h"
#include "catapult/builders/TransferBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ConversionExtensions.h"
#include "catapult/extensions/IdGenerator.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/version/version.h"
#include "StateBroker.h"
#include "catapult/io/BufferedFileStream.h"
#include <inttypes.h>



namespace catapult { namespace tools { namespace nemgen {

	namespace {
		std::string FixName(const std::string& mosaicName) {
			auto name = mosaicName;
			for (auto& ch : name) {
				if (':' == ch)
					ch = '.';
			}

			return name;
		}

		model::MosaicFlags GetFlags(const model::MosaicProperties& properties) {
			auto flags = model::MosaicFlags::None;
			auto allFlags = std::initializer_list<model::MosaicFlags>{
				model::MosaicFlags::Supply_Mutable, model::MosaicFlags::Transferable
			};

			for (auto flag : allFlags) {
				if (properties.is(flag))
					flags |= flag;
			}

			return flags;
		}
	}


	void NemesisTransactions::addRegisterNamespace(const std::string& namespaceName, BlockDuration duration) {
		builders::RegisterNamespaceBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
		builder.setDuration(duration);
		signAndAdd(builder.build());
	}
	void NemesisTransactions::addRegisterNamespace(const std::string& namespaceName, NamespaceId parentId) {
		builders::RegisterNamespaceBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
		builder.setParentId(parentId);
		signAndAdd(builder.build());
	}
	MosaicId NemesisTransactions::addMosaicDefinition(MosaicNonce nonce, const model::MosaicProperties& properties) {
		builders::MosaicDefinitionBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setMosaicNonce(nonce);
		builder.setFlags(GetFlags(properties));
		builder.setDivisibility(properties.divisibility());
		if (Eternal_Artifact_Duration != properties.duration())
			builder.addProperty({ model::MosaicPropertyId::Duration, properties.duration().unwrap() });

		auto pTransaction = builder.build();
		auto id = pTransaction->MosaicId;
		signAndAdd(std::move(pTransaction));
		return id;
	}
	UnresolvedMosaicId NemesisTransactions::addMosaicAlias(const std::string& mosaicName, MosaicId mosaicId) {
		auto namespaceName = FixName(mosaicName);
		auto namespacePath = extensions::GenerateNamespacePath(namespaceName);
		auto namespaceId = namespacePath[namespacePath.size() - 1];
		builders::MosaicAliasBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setNamespaceId(namespaceId);
		builder.setMosaicId(mosaicId);
		auto pTransaction = builder.build();
		signAndAdd(std::move(pTransaction));

		CATAPULT_LOG(debug)
			<< "added alias from ns " << utils::HexFormat(namespaceId) << " (" << namespaceName
			<< ") -> mosaic " << utils::HexFormat(mosaicId);
		return UnresolvedMosaicId(namespaceId.unwrap());
	}
	void NemesisTransactions::addMosaicSupplyChange(UnresolvedMosaicId mosaicId, Amount delta) {
		builders::MosaicSupplyChangeBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setMosaicId(mosaicId);
		builder.setDirection(model::MosaicSupplyChangeDirection::Increase);
		builder.setDelta(delta);
		auto pTransaction = builder.build();
		signAndAdd(std::move(pTransaction));
	}
	void NemesisTransactions::addTransfer(
			const std::map<std::string, UnresolvedMosaicId>& mosaicNameToMosaicIdMap,
			const Address& recipientAddress,
			const std::vector<MosaicSeed>& seeds) {
		auto recipientUnresolvedAddress = extensions::CopyToUnresolvedAddress(recipientAddress);
		builders::TransferBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setRecipient(recipientUnresolvedAddress);
		for (const auto& seed : seeds) {
			auto mosaicId = mosaicNameToMosaicIdMap.at(seed.Name);
			builder.addMosaic({ mosaicId, seed.Amount });
		}

		signAndAdd(builder.build());
	}
	void NemesisTransactions::addConfig(const std::string& resourcesPath) {
		builders::NetworkConfigBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setApplyHeightDelta(BlockDuration{0});
		auto resPath = boost::filesystem::path(resourcesPath);
		builder.setBlockChainConfig((resPath / "resources/config-network.properties").generic_string());
		builder.setSupportedVersionsConfig((resPath / "resources/supported-entities.json").generic_string());

		signAndAdd(builder.build());
	}
	void NemesisTransactions::addUpgrade() {
		builders::BlockchainUpgradeBuilder builder(m_Config->NetworkIdentifier, m_signer.publicKey());
		builder.setUpgradePeriod(BlockDuration{0});
		builder.setNewBlockchainVersion(version::CurrentBlockchainVersion);

		signAndAdd(builder.build());
	}
	void NemesisTransactions::addHarvester(const std::string& harvesterPrivateKey) {
		auto signer = crypto::KeyPair::FromString(harvesterPrivateKey);
		builders::AddHarvesterBuilder builder(m_Config->NetworkIdentifier, signer.publicKey(), signer.publicKey());

		signAndAdd(builder.build(), signer);
	}
	const std::vector<TransactionHashContainer>& NemesisTransactions::transactions() const {
		return m_transactions;
	}
	void NemesisTransactions::signAndAdd(model::UniqueEntityPtr<model::Transaction>&& pTransaction) {
		signAndAdd(std::move(pTransaction), m_signer);
	}

	void NemesisTransactions::signAndAdd(
			model::UniqueEntityPtr<model::Transaction>&& pTransaction,
			const crypto::KeyPair& signer) {
		pTransaction->Deadline = Timestamp(1);
		extensions::TransactionExtensions(m_Config->NemesisGenerationHash).sign(signer, *pTransaction);
		m_BufferSize += pTransaction->Size;
		m_TotalTransactions++;
		m_totalSize += pTransaction->Size;
		auto transactionHash = model::CalculateHash(*pTransaction, m_Config->NemesisGenerationHash);
		m_builder.update(transactionHash);
		TransactionHashContainer container;
		container.transaction = std::move(pTransaction);

		const auto& plugin = *m_Registry.findPlugin(container.transaction->Type);
		container.entityHash = model::CalculateHash(*container.transaction, m_Config->NemesisGenerationHash, plugin.dataBuffer(*container.transaction));
		container.merkleHash = model::CalculateMerkleComponentHash(*container.transaction, container.entityHash, m_Registry);

		m_transactions.push_back(container);
		trySpool();
	}
	void NemesisTransactions::trySpool() {
		if(m_BufferSize >= m_Config->TxBufferSize && m_Config->EnableSpool) {
			spool();
		}
	}

	void NemesisTransactions::finalize() {
		if(m_BufferSize >= 0 && m_Config->EnableSpool) {
			spool();
		}
		m_builder.final(m_hash);
		m_spoolFile.reset();
	}

	uint32_t NemesisTransactions::Size() const {
		return m_totalSize;
	}
	Hash256 NemesisTransactions::TxHash() const {
		return m_hash;
	}
	void NemesisTransactions::spool() {
		if(!m_spoolFile)
			return;
		for(auto tx : m_transactions) {
			m_spoolFile->write({ reinterpret_cast<const uint8_t*>(tx.transaction.get()), tx.transaction->Size });
			m_spoolFile->write({ reinterpret_cast<const uint8_t*>(tx.entityHash.data()), Hash256_Size });
			m_spoolFile->write({ reinterpret_cast<const uint8_t*>(tx.merkleHash.data()), Hash256_Size });
		}
		m_transactions.clear();
		m_spoolFile->flush();
		m_BufferSize = 0;
	}

	NemesisTransactions::NemesisTransactions(
			const crypto::KeyPair& signer,
			const NemesisConfiguration& config,
			const model::TransactionRegistry& registry)
		: m_signer(signer)
		, m_Config(&config)
		, m_Registry(registry)
		, m_totalSize(0)
		, m_TotalTransactions(0)
	{
		if(config.EnableSpool){
			boost::filesystem::path path = m_Config->TransactionsPath;
			if (!boost::filesystem::exists(path))
				boost::filesystem::create_directory(path);
			path /= "txspool.dat";
			if(boost::filesystem::exists(path)) {
				boost::filesystem::remove(path);
			}
			m_spoolFile = std::make_unique<io::BufferedOutputFileStream>(io::RawFile(path.generic_string(), io::OpenMode::Read_Write));
		}
	}
	NemesisTransactions::NemesisTransactionsView NemesisTransactions::createView() const {
		if(m_spoolFile) {
			CATAPULT_THROW_RUNTIME_ERROR("Attempting to create transactions view in prior to finalization.")
		}
		boost::filesystem::path path = m_Config->TransactionsPath;
		path /= "txspool.dat";
		return NemesisTransactions::NemesisTransactionsView(io::RawFile(path.generic_string(), io::OpenMode::Read_Only));
	}
	uint32_t NemesisTransactions::Total() const {
		return m_TotalTransactions;
	}

}}}
