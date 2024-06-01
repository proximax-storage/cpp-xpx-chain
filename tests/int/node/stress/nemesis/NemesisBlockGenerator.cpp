/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NemesisBlockGenerator.h"
#include "NemesisConfiguration.h"
#include "sdk/src/builders/AddHarvesterBuilder.h"
#include "sdk/src/builders/NetworkConfigBuilder.h"
#include "sdk/src/builders/BlockchainUpgradeBuilder.h"
#include "sdk/src/builders/MosaicAliasBuilder.h"
#include "sdk/src/builders/MosaicDefinitionBuilder.h"
#include "sdk/src/builders/MosaicSupplyChangeBuilder.h"
#include "sdk/src/builders/RegisterNamespaceBuilder.h"
#include "sdk/src/builders/TransferBuilder.h"
#include "catapult/crypto/KeyPair.h"
#include "sdk/src/extensions/BlockExtensions.h"
#include "sdk/src/extensions/IdGenerator.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/io/FileBlockStorage.h"
#include "catapult/model/Address.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/TransactionPlugin.h"
#include "catapult/version/version.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/constants.h"
#include "tools/nemgen/blockhashes/NemesisBlockHashesCalculator.h"
#include "plugins/txes/committee/src/plugins/AddHarvesterTransactionPlugin.h"
#include "plugins/txes/config/src/plugins/NetworkConfigTransactionPlugin.h"
#include "plugins/txes/upgrade/src/plugins/BlockchainUpgradeTransactionPlugin.h"
#include "plugins/txes/mosaic/src/config/MosaicConfiguration.h"
#include "plugins/txes/mosaic/src/plugins/MosaicDefinitionTransactionPlugin.h"
#include "plugins/txes/mosaic/src/plugins/MosaicSupplyChangeTransactionPlugin.h"
#include "plugins/txes/namespace/src/config/NamespaceConfiguration.h"
#include "plugins/txes/namespace/src/plugins/MosaicAliasTransactionPlugin.h"
#include "plugins/txes/namespace/src/plugins/RegisterNamespaceTransactionPlugin.h"
#include "plugins/txes/transfer/src/plugins/TransferTransactionPlugin.h"
#include <utility>

namespace catapult { namespace test {

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
				builder.setDuration(std::move(duration));
				signAndAdd(builder.build());
			}

			void addRegisterNamespace(const std::string& namespaceName, NamespaceId parentId) {
				builders::RegisterNamespaceBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setName({ reinterpret_cast<const uint8_t*>(namespaceName.data()), namespaceName.size() });
				builder.setParentId(std::move(parentId));
				signAndAdd(builder.build());
			}

			MosaicId addMosaicDefinition(MosaicNonce nonce, const model::MosaicProperties& properties) {
				builders::MosaicDefinitionBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setMosaicNonce(std::move(nonce));
				builder.setFlags(GetFlags(properties));
				builder.setDivisibility(properties.divisibility());
				if (Eternal_Artifact_Duration != properties.duration())
					builder.addProperty({ model::MosaicPropertyId::Duration, properties.duration().unwrap() });

				auto pTransaction = builder.build();
				auto id = pTransaction->MosaicId;
				signAndAdd(std::move(pTransaction));
				return id;
			}

			UnresolvedMosaicId addMosaicAlias(const std::string& mosaicName, const MosaicId& mosaicId) {
				auto namespaceName = FixName(mosaicName);
				auto namespacePath = extensions::GenerateNamespacePath(namespaceName);
				auto namespaceId = namespacePath[namespacePath.size() - 1];
				builders::MosaicAliasBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setNamespaceId(namespaceId);
				builder.setMosaicId(mosaicId);
				auto pTransaction = builder.build();
				signAndAdd(std::move(pTransaction));

				return UnresolvedMosaicId(namespaceId.unwrap());
			}

			void addMosaicSupplyChange(UnresolvedMosaicId mosaicId, Amount delta) {
				builders::MosaicSupplyChangeBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setMosaicId(std::move(mosaicId));
				builder.setDirection(model::MosaicSupplyChangeDirection::Increase);
				builder.setDelta(std::move(delta));
				auto pTransaction = builder.build();
				signAndAdd(std::move(pTransaction));
			}

			void addTransfer(const std::map<std::string, UnresolvedMosaicId>& mosaicNameToMosaicIdMap, const UnresolvedAddress& recipientAddress, const std::vector<MosaicSeed>& seeds) {
				builders::TransferBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setRecipient(recipientAddress);
				for (const auto& seed : seeds) {
					const auto& mosaicId = mosaicNameToMosaicIdMap.at(seed.Name);
					builder.addMosaic({ mosaicId, seed.Amount });
				}

				signAndAdd(builder.build());
			}

			void addConfig(const std::string& resourcesPath) {
				builders::NetworkConfigBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setApplyHeightDelta(BlockDuration{0});
				auto resPath = boost::filesystem::path(resourcesPath);
				builder.setBlockChainConfig((resPath / "config-network.properties").generic_string());
				builder.setSupportedVersionsConfig((resPath / "supported-entities.json").generic_string());

				signAndAdd(builder.build());
			}

			void addUpgrade() {
				builders::BlockchainUpgradeBuilder builder(m_networkIdentifier, m_signer.publicKey());
				builder.setUpgradePeriod(BlockDuration{0});
				builder.setNewBlockchainVersion(version::CurrentBlockchainVersion);

				signAndAdd(builder.build());
			}

			void addHarvester(const crypto::KeyPair& harvesterKeyPair) {
				builders::AddHarvesterBuilder builder(m_networkIdentifier, harvesterKeyPair.publicKey(), harvesterKeyPair.publicKey());

				signAndAdd(builder.build(), harvesterKeyPair);
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

		model::UniqueEntityPtr<model::Block> CreateNemesisBlock(const NemesisConfiguration& config, const std::string& resourcesPath) {
			NemesisTransactions transactions(config.NetworkIdentifier, config.NemesisGenerationHash, config.NemesisSignerKeyPair);

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
			for (const auto& [mosaicName, mosaicEntry] : config.MosaicEntries) {
				// - definition
				auto mosaicId = transactions.addMosaicDefinition(nonce, mosaicEntry.definition().properties());
				nonce = nonce + MosaicNonce(1);

				// - alias
				auto unresolvedMosaicId = transactions.addMosaicAlias(mosaicName, mosaicId);
				nameToMosaicIdMap.emplace(mosaicName, unresolvedMosaicId);

				// - supply
				transactions.addMosaicSupplyChange(unresolvedMosaicId, mosaicEntry.supply());
			}

			// - mosaic distribution
			for (const auto& [address, seeds] : config.NemesisAddressToMosaicSeeds)
				transactions.addTransfer(nameToMosaicIdMap, address, seeds);

			transactions.addConfig(resourcesPath);
			transactions.addUpgrade();

			for (const auto& key : config.HarvesterKeyPairs)
				transactions.addHarvester(key);

			model::PreviousBlockContext context;
			auto pBlock = model::CreateBlock(context, config.NetworkIdentifier, config.NemesisSignerKeyPair.publicKey(), transactions.transactions());
			pBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
			pBlock->Type = model::Entity_Type_Nemesis_Block;
			pBlock->FeeInterest = 1;
			pBlock->FeeInterestDenominator = 1;
			pBlock->Timestamp = utils::NetworkTime();
			extensions::BlockExtensions(config.NemesisGenerationHash).signFullBlock(config.NemesisSignerKeyPair, *pBlock);
			return pBlock;
		}

		model::TransactionRegistry CreateTransactionRegistry() {
			auto mosaicConfig = config::MosaicConfiguration::Uninitialized();
			auto namespaceConfig = config::NamespaceConfiguration::Uninitialized();
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.SetPluginConfiguration(mosaicConfig);
			networkConfig.SetPluginConfiguration(namespaceConfig);

			config::BlockchainConfiguration config{
				config::ImmutableConfiguration::Uninitialized(),
				std::move(networkConfig),
				config::NodeConfiguration::Uninitialized(),
				config::LoggingConfiguration::Uninitialized(),
				config::UserConfiguration::Uninitialized(),
				config::ExtensionsConfiguration::Uninitialized(),
				config::InflationConfiguration::Uninitialized(),
				config::SupportedEntityVersions()
			};

			auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);
			model::TransactionRegistry registry;
			registry.registerPlugin(plugins::CreateMosaicAliasTransactionPlugin(pConfigHolder));
			registry.registerPlugin(plugins::CreateMosaicDefinitionTransactionPlugin(pConfigHolder));
			registry.registerPlugin(plugins::CreateMosaicSupplyChangeTransactionPlugin(pConfigHolder));
			registry.registerPlugin(plugins::CreateRegisterNamespaceTransactionPlugin(pConfigHolder));
			registry.registerPlugin(plugins::CreateTransferTransactionPlugin());
			registry.registerPlugin(plugins::CreateNetworkConfigTransactionPlugin());
			registry.registerPlugin(plugins::CreateBlockchainUpgradeTransactionPlugin());
			registry.registerPlugin(plugins::CreateAddHarvesterTransactionPlugin());
			return registry;
		}

		model::BlockElement CreateNemesisBlockElement(const NemesisConfiguration& config, const model::Block& block) {
			auto registry = CreateTransactionRegistry();
			auto generationHash = config.NemesisGenerationHash;
			return extensions::BlockExtensions(generationHash, registry).convertBlockToBlockElement(block, generationHash);
		}
	}

	void GenerateAndSaveNemesisBlock(const std::string& resourcesPath, const NemesisConfiguration& nemesisConfig) {
		// 1. load config
		auto config = config::BlockchainConfiguration::LoadFromPath(boost::filesystem::path(resourcesPath), "server");
		auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);

		// 2. create the nemesis block element
		auto pBlock = CreateNemesisBlock(nemesisConfig, resourcesPath);
		auto blockElement = CreateNemesisBlockElement(nemesisConfig, *pBlock);
		auto blockExecutionHashesInfo = tools::nemgen::CalculateNemesisBlockExecutionHashes(blockElement, pConfigHolder);

		// 3. update block with result of execution
		pBlock->BlockReceiptsHash = blockExecutionHashesInfo.ReceiptsHash;
		pBlock->StateHash = blockExecutionHashesInfo.StateHash;
		extensions::BlockExtensions(nemesisConfig.NemesisGenerationHash).signFullBlock(nemesisConfig.NemesisSignerKeyPair, *pBlock);
		blockElement.EntityHash = model::CalculateHash(*pBlock);

		// 4. save the nemesis block element
		auto binDirectoryPath = boost::filesystem::path(nemesisConfig.BinDirectory);
		boost::filesystem::create_directories(binDirectoryPath / "00000");
		{
			io::RawFile file((binDirectoryPath / "00000" / "hashes.dat").generic_string(), io::OpenMode::Read_Write);
			file.write(std::vector<uint8_t>(2 * Hash256_Size));
		}
		io::IndexFile((binDirectoryPath / "index.dat").generic_string()).set(0);
		io::FileBlockStorage storage(nemesisConfig.BinDirectory);
		storage.saveBlock(blockElement);
	}
}}
