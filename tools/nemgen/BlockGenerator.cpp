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

#include "BlockGenerator.h"
#include "NemesisConfiguration.h"
#include "NemesisExecutionHasher.h"
#include "TransactionRegistryFactory.h"
#include "catapult/builders/AddHarvesterBuilder.h"
#include "catapult/builders/NetworkConfigBuilder.h"
#include "catapult/builders/BlockchainUpgradeBuilder.h"
#include "catapult/builders/MosaicAliasBuilder.h"
#include "catapult/builders/MosaicDefinitionBuilder.h"
#include "catapult/builders/MosaicSupplyChangeBuilder.h"
#include "catapult/builders/RegisterNamespaceBuilder.h"
#include "catapult/builders/TransferBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/BlockExtensions.h"
#include "catapult/extensions/ConversionExtensions.h"
#include "catapult/extensions/IdGenerator.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/model/Address.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/version/version.h"
#include "catapult/utils/NetworkTime.h"

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

		std::string GetChildName(const std::string& namespaceName) {
			return namespaceName.substr(namespaceName.rfind('.') + 1);
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

		class NemesisTransactions {
		public:
			NemesisTransactions(
					model::NetworkIdentifier networkIdentifier,
					const GenerationHash& generationHash,
					const crypto::KeyPair& signer)
					: m_networkIdentifier(networkIdentifier)
					, m_generationHash(generationHash)
					, m_signer(signer)
			{}

		public:
			void addRegisterNamespace(const std::string& namespaceName, BlockDuration duration) {
				builders::RegisterNamespaceBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
				builder.setDuration(duration);
				signAndAdd(builder.build());
			}

			void addRegisterNamespace(const std::string& namespaceName, NamespaceId parentId) {
				builders::RegisterNamespaceBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
				builder.setParentId(parentId);
				signAndAdd(builder.build());
			}

			MosaicId addMosaicDefinition(MosaicNonce nonce, const model::MosaicProperties& properties) {
				builders::MosaicDefinitionBuilder builder(m_networkIdentifier, m_signer.publicKey());
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

			UnresolvedMosaicId addMosaicAlias(const std::string& mosaicName, MosaicId mosaicId) {
				auto namespaceName = FixName(mosaicName);
				auto namespacePath = extensions::GenerateNamespacePath(namespaceName);
				auto namespaceId = namespacePath[namespacePath.size() - 1];
				builders::MosaicAliasBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setNamespaceId(namespaceId);
				builder.setMosaicId(mosaicId);
				auto pTransaction = builder.build();
				signAndAdd(std::move(pTransaction));

				CATAPULT_LOG(debug)
						<< "added alias from ns " << utils::HexFormat(namespaceId) << " (" << namespaceName
						<< ") -> mosaic " << utils::HexFormat(mosaicId);
				return UnresolvedMosaicId(namespaceId.unwrap());
			}

			void addMosaicSupplyChange(UnresolvedMosaicId mosaicId, Amount delta) {
				builders::MosaicSupplyChangeBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setMosaicId(mosaicId);
				builder.setDirection(model::MosaicSupplyChangeDirection::Increase);
				builder.setDelta(delta);
				auto pTransaction = builder.build();
				signAndAdd(std::move(pTransaction));
			}

			void addTransfer(
					const std::map<std::string, UnresolvedMosaicId>& mosaicNameToMosaicIdMap,
					const Address& recipientAddress,
					const std::vector<MosaicSeed>& seeds) {
				auto recipientUnresolvedAddress = extensions::CopyToUnresolvedAddress(recipientAddress);
				builders::TransferBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setRecipient(recipientUnresolvedAddress);
				for (const auto& seed : seeds) {
					auto mosaicId = mosaicNameToMosaicIdMap.at(seed.Name);
					builder.addMosaic({ mosaicId, seed.Amount });
				}

				signAndAdd(builder.build());
			}

			void addConfig(const std::string& resourcesPath) {
				builders::NetworkConfigBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setApplyHeightDelta(BlockDuration{0});
				auto resPath = boost::filesystem::path(resourcesPath);
				builder.setBlockChainConfig((resPath / "resources/config-network.properties").generic_string());
				builder.setSupportedVersionsConfig((resPath / "resources/supported-entities.json").generic_string());

				signAndAdd(builder.build());
			}

			void addUpgrade() {
				builders::BlockchainUpgradeBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setUpgradePeriod(BlockDuration{0});
				builder.setNewBlockchainVersion(version::CurrentBlockchainVersion);

				signAndAdd(builder.build());
			}

			void addHarvester(const std::string& harvesterPrivateKey) {
				auto signer = crypto::KeyPair::FromString(harvesterPrivateKey);
				builders::AddHarvesterBuilder builder(m_networkIdentifier, signer.publicKey(), signer.publicKey());

				signAndAdd(builder.build(), signer);
			}

		public:
			const model::Transactions& transactions() const {
				return m_transactions;
			}

		private:
			void signAndAdd(model::UniqueEntityPtr<model::Transaction>&& pTransaction) {
				signAndAdd(std::move(pTransaction), m_signer);
			}

			void signAndAdd(model::UniqueEntityPtr<model::Transaction>&& pTransaction, const crypto::KeyPair& signer) {
				pTransaction->Deadline = Timestamp(1);
				extensions::TransactionExtensions(m_generationHash).sign(signer, *pTransaction);
				m_transactions.push_back(std::move(pTransaction));
			}

		private:
			model::NetworkIdentifier m_networkIdentifier;
			const GenerationHash& m_generationHash;
			const crypto::KeyPair& m_signer;
			model::Transactions m_transactions;
		};
	}

	model::UniqueEntityPtr<model::Block> CreateNemesisBlock(const NemesisConfiguration& config, const std::string& resourcesPath) {
		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);
		NemesisTransactions transactions(config.NetworkIdentifier, config.NemesisGenerationHash, signer);

		// - namespace creation
		for (const auto& rootPair : config.RootNamespaces) {
			// - root
			const auto& root = rootPair.second;
			const auto& rootName = config.NamespaceNames.at(root.id());
			auto duration = std::numeric_limits<BlockDuration::ValueType>::max() == root.lifetime().End.unwrap()
					? Eternal_Artifact_Duration
					: BlockDuration((root.lifetime().End - root.lifetime().Start).unwrap());
			transactions.addRegisterNamespace(rootName, duration);

			// - children
			std::map<size_t, std::vector<state::Namespace::Path>> paths;
			for (const auto& childPair : root.children()) {
				const auto& path = childPair.second.Path;
				paths[path.size()].push_back(path);
			}

			for (const auto& pair : paths) {
				for (const auto& path : pair.second) {
					const auto& child = state::Namespace(path);
					auto subName = GetChildName(config.NamespaceNames.at(child.id()));
					transactions.addRegisterNamespace(subName, child.parentId());
				}
			}
		}

		// - mosaic creation
		MosaicNonce nonce;
		std::map<std::string, UnresolvedMosaicId> nameToMosaicIdMap;
		for (const auto& mosaicPair : config.MosaicEntries) {
			const auto& mosaicEntry = mosaicPair.second;

			// - definition
			auto mosaicId = transactions.addMosaicDefinition(nonce, mosaicEntry.definition().properties());
			CATAPULT_LOG(debug) << "mapping " << mosaicPair.first << " to " << utils::HexFormat(mosaicId) << " (nonce " << nonce << ")";
			nonce = nonce + MosaicNonce(1);

			// - alias
			auto unresolvedMosaicId = transactions.addMosaicAlias(mosaicPair.first, mosaicId);
			nameToMosaicIdMap.emplace(mosaicPair.first, unresolvedMosaicId);

			// - supply
			transactions.addMosaicSupplyChange(unresolvedMosaicId, mosaicEntry.supply());
		}

		// - mosaic distribution
		for (const auto& addressMosaicSeedsPair : config.NemesisAddressToMosaicSeeds) {
			auto recipient = model::StringToAddress(addressMosaicSeedsPair.first);
			transactions.addTransfer(nameToMosaicIdMap, recipient, addressMosaicSeedsPair.second);
		}

		transactions.addConfig(resourcesPath);
		transactions.addUpgrade();

		for (const auto& key : config.HarvesterPrivateKeys) {
			transactions.addHarvester(key);
		}

		model::PreviousBlockContext context;
		auto pBlock = model::CreateBlock(context, config.NetworkIdentifier, signer.publicKey(), transactions.transactions());
		pBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
		pBlock->Type = model::Entity_Type_Nemesis_Block;
		pBlock->FeeInterest = 1;
		pBlock->FeeInterestDenominator = 1;
		pBlock->Timestamp = utils::NetworkTime();
		extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, *pBlock);
		return pBlock;
	}

	Hash256 UpdateNemesisBlock(
			const NemesisConfiguration& config,
			model::Block& block,
			NemesisExecutionHashesDescriptor& executionHashesDescriptor) {
		block.BlockReceiptsHash = executionHashesDescriptor.ReceiptsHash;
		block.StateHash = executionHashesDescriptor.StateHash;

		auto signer = crypto::KeyPair::FromString(config.NemesisSignerPrivateKey);
		extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(signer, block);
		return model::CalculateHash(block);
	}

	model::BlockElement CreateNemesisBlockElement(const NemesisConfiguration& config, const model::Block& block) {
		auto registry = CreateTransactionRegistry();
		auto generationHash = config.NemesisGenerationHash;
		return extensions::BlockExtensions(generationHash, registry).convertBlockToBlockElement(block, generationHash);
	}
}}}
