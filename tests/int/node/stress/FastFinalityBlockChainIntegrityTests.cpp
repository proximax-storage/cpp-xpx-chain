/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/crypto/KeyUtils.h"
#include "catapult/model/Address.h"
#include "catapult/model/TransactionFeeCalculator.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/int/node/stress/nemesis/NemesisBlockGenerator.h"
#include "tests/int/node/stress/test/BlockChainBuilder.h"
#include "tests/int/node/test/LocalNodeRequestTestUtils.h"
#include "tests/int/node/test/LocalNodeTestContext.h"
#include "tests/test/nodeps/Logging.h"
#include <boost/property_tree/ini_parser.hpp>
#include <boost/thread.hpp>

namespace pt = boost::property_tree;

namespace catapult { namespace local {

#define TEST_CLASS FastFinalityBlockChainIntegrityTests

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
		constexpr size_t Network_Size = 30;
		constexpr size_t Bootstrap_Node_Count = 6;
		constexpr uint64_t Mosaic_Supply = 8'999'999'998'000'000;
		constexpr Height Max_Chain_Height = Height(10);
		constexpr auto Process_Name = "server";

		void RecursiveCopy(const boost::filesystem::path& source, const boost::filesystem::path& destination) {
			if (boost::filesystem::is_directory(source)) {
				boost::filesystem::create_directories(destination);
				for (const auto& item : boost::filesystem::directory_iterator(source))
					RecursiveCopy(item.path(), destination / item.path().filename());
			}
			else if (boost::filesystem::is_regular_file(source)) {
				boost::filesystem::copy(source, destination);
			}
		}

		uint16_t GetPortForNode(uint32_t id) {
			return static_cast<uint16_t>(20000 + 5 * id);
		}

		uint16_t GetDbrbPortForNode(uint32_t id) {
			return GetPortForNode(id) + 3;
		}

		void SetAutoHarvesting(const std::string& configFilePath, const std::string& harvestKey) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);
			properties.put("harvesting.harvestKey", harvestKey);
			pt::write_ini(configFilePath, properties);
		}

		void SetNodeProperties(const std::string& configFilePath, uint32_t id) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);
			auto port = GetPortForNode(id);
			properties.put("node.port", port);
			properties.put("node.apiPort", port + 1);
			properties.put("node.dbrbPort", port + 3);
			properties.put("localnode.friendlyName", "NODE " + std::to_string(id));
			pt::write_ini(configFilePath, properties);
		}

		void SetUserProperties(const std::string& configFilePath, const std::string& dataPath, const std::string& bootPrivateKey) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);
			properties.put("account.bootKey", bootPrivateKey);
			properties.put("storage.dataDirectory", dataPath);
			pt::write_ini(configFilePath, properties);
		}

		void PrepareConfiguration(const std::string& destination, const std::string& seedPath, uint32_t id, const std::string& bootPrivateKey, const std::string& harvestingPrivateKey) {
			auto sourcePath = boost::filesystem::path(seedPath);
			auto destinationPath = boost::filesystem::path(destination);
			auto dataPath = destinationPath / "data";
			auto resourcesPath = destinationPath / "resources";

			RecursiveCopy(sourcePath / "data", dataPath);
			RecursiveCopy(sourcePath / "resources", resourcesPath);

			auto harvestingConfigPath = resourcesPath / "config-harvesting.properties";
			SetAutoHarvesting(harvestingConfigPath.generic_string(), harvestingPrivateKey);

			auto nodeConfigPath = resourcesPath / "config-node.properties";
			SetNodeProperties(nodeConfigPath.generic_string(), id);

			auto userConfigPath = resourcesPath / "config-user.properties";
			SetUserProperties(userConfigPath.generic_string(), dataPath.generic_string(), bootPrivateKey);
		}

		class LocalNodeTestContext {
		public:
			LocalNodeTestContext(
					const std::vector<ionet::Node>& bootstrapNodes,
					const std::vector<crypto::KeyPair>& bootKeys,
					const std::vector<crypto::KeyPair>& harvesterKeys,
					uint32_t id,
					const std::string& seedPath)
				: m_bootstrapNodes(bootstrapNodes)
				, m_bootKeys(bootKeys)
				, m_harvesterKeys(harvesterKeys)
				, m_tempDir("lntc_" + std::to_string(id))
				, m_id(id)
				, m_seedDataPath(seedPath)
				, m_booted(false)
			{}

		public:
			/// Gets the data directory.
			std::string dataDirectory() const {
				return m_tempDir.name();
			}

			/// Gets the resources directory.
			std::string resourcesDirectory() const {
				return m_tempDir.name() + "/resources";
			}

			/// Gets the primary (first) local node.
			local::LocalNode& localNode() const {
				return *m_pLocalNode;
			}

			bool isBooted() const {
				return m_booted;
			}

			/// Boots a new local node.
			void boot() {
				if (m_pLocalNode)
					CATAPULT_THROW_RUNTIME_ERROR("cannot boot local node multiple times via same test context");

				PrepareConfiguration(
					dataDirectory(),
					m_seedDataPath,
					m_id,
					crypto::FormatKeyAsString(m_bootKeys.at(m_id).privateKey()),
					crypto::FormatKeyAsString(m_harvesterKeys.at(m_id).privateKey()));

				auto config = config::BlockchainConfiguration::LoadFromPath(resourcesDirectory(), Process_Name);
				auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);

				auto pBootstrapper = std::make_unique<extensions::ProcessBootstrapper>(
					pConfigHolder,
					resourcesDirectory(),
					extensions::ProcessDisposition::Production,
					"LocalNodeTests");
				pBootstrapper->addStaticNodes(m_bootstrapNodes);
				pBootstrapper->loadExtensions();

				m_pLocalNode = local::CreateLocalNode(m_bootKeys.at(m_id), std::move(pBootstrapper));
				m_booted = true;
			}

			/// Resets this context and shuts down the local node.
			void reset() {
				m_pLocalNode->shutdown();
				m_pLocalNode.reset();
			}

		private:
			const std::vector<ionet::Node>& m_bootstrapNodes;
			const std::vector<crypto::KeyPair>& m_bootKeys;
			const std::vector<crypto::KeyPair>& m_harvesterKeys;
			test::TempDirectoryGuard m_tempDir;
			uint32_t m_id;
			std::string m_seedDataPath;
			std::unique_ptr<local::LocalNode> m_pLocalNode;
			std::atomic_bool m_booted;
		};

		ionet::Node CreateNode(uint32_t id, const std::vector<crypto::KeyPair>& bootKeys) {
			auto metadata = ionet::NodeMetadata(model::NetworkIdentifier::Mijin_Test, "NODE " + std::to_string(id));
			metadata.Roles = ionet::NodeRoles::Peer;
			return {
				bootKeys.at(id).publicKey(),
				test::CreateLocalHostNodeEndpoint(GetPortForNode(id), GetDbrbPortForNode(id)),
				metadata
			};
		}

		std::vector<ionet::Node> CreateBootstrapNodes(const std::vector<crypto::KeyPair>& bootKeys, size_t count) {
			std::vector<ionet::Node> nodes;
			for (auto i = 0u; i < count; ++i)
				nodes.push_back(CreateNode(i, bootKeys));

			return nodes;
		}

		struct LocalNodeStatistics {
			model::ChainScore Score;
			Hash256 StateHash;
			catapult::Height Height;
		};

		LocalNodeStatistics GetStatistics(const LocalNodeTestContext& context) {
			LocalNodeStatistics stats;

			const auto& cacheView = context.localNode().cache().createView();
			stats.Score = context.localNode().score();
			stats.StateHash = cacheView.calculateStateHash().StateHash;
			stats.Height = cacheView.height();

			return stats;
		}

		bool IsMaxHeightReached(const std::vector<std::unique_ptr<LocalNodeTestContext>>& contexts) {
			for (const auto& pContext : contexts) {
				if (!pContext->isBooted() || pContext->localNode().state().storage().view().chainHeight() < Max_Chain_Height)
					return false;
			}

			return true;
		}

		bool TryWaitForMaxHeight(const std::vector<std::unique_ptr<LocalNodeTestContext>>& contexts, size_t timeoutSeconds) {
			auto begin = std::chrono::high_resolution_clock::now();

			while (!IsMaxHeightReached(contexts)) {
				auto current = std::chrono::high_resolution_clock::now();
				auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(current - begin).count();
				if (static_cast<size_t>(elapsedSeconds) > timeoutSeconds)
					return false;

				std::this_thread::sleep_for(std::chrono::seconds(1));
			}

			return true;
		}

		std::string ToString(const MosaicId& mosaicId) {
			std::ostringstream out;
			out << "0x" << std::uppercase << std::hex << mosaicId.unwrap();
			return out.str();
		}

		void UpdateImmutableConfiguration(const std::string& configFilePath, const test::NemesisConfiguration& nemesisConfig) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);

			properties.put("immutable.generationHash", test::ToString(nemesisConfig.NemesisGenerationHash));
			auto currencyMosaicId = ToString(nemesisConfig.MosaicEntries.at("prx.xpx").mosaicId());
			properties.put("immutable.currencyMosaicId", currencyMosaicId);
			properties.put("immutable.harvestingMosaicId", currencyMosaicId);
			properties.put("immutable.storageMosaicId", ToString(nemesisConfig.MosaicEntries.at("prx.so").mosaicId()));
			properties.put("immutable.streamingMosaicId", ToString(nemesisConfig.MosaicEntries.at("prx.sm").mosaicId()));
			properties.put("immutable.superContractMosaicId", ToString(nemesisConfig.MosaicEntries.at("prx.sc").mosaicId()));
			properties.put("immutable.reviewMosaicId", ToString(nemesisConfig.MosaicEntries.at("prx.rw").mosaicId()));
			properties.put("immutable.xarMosaicId", ToString(nemesisConfig.MosaicEntries.at("prx.xar").mosaicId()));
			properties.put("immutable.initialCurrencyAtomicUnits", test::ToString(Mosaic_Supply));

			pt::write_ini(configFilePath, properties);
		}

		void UpdateNetworkConfiguration(const std::string& configFilePath, const Key& nemesisKey, const std::vector<crypto::KeyPair>& bootKeys, const std::vector<crypto::KeyPair>& harvesterKeys) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);

			properties.put("network.publicKey", test::ToString(nemesisKey));
			auto& bootstrapHarvesters = properties.get_child(pt::ptree::path_type("bootstrap.harvesters", '/'));
			bootstrapHarvesters.clear();
			for (auto i = 0u; i < Bootstrap_Node_Count; ++i)
				bootstrapHarvesters.add(test::ToString(bootKeys[i].publicKey()), test::ToString(harvesterKeys[i].publicKey()));

			pt::write_ini(configFilePath, properties);
		}

		void AssertNetworkBuildsChainSuccess() {
			// Arrange
			std::vector<crypto::KeyPair> bootKeys;
			std::vector<crypto::KeyPair> harvesterKeys;
			for (auto i = 0u; i < Network_Size; ++i) {
				bootKeys.push_back(test::GenerateKeyPair());
				harvesterKeys.push_back(test::GenerateKeyPair());
			}

			test::TempDirectoryGuard seedDir("seed");
			auto seedPath = boost::filesystem::path(seedDir.name());
			auto resourcesPath = seedPath / "resources";
			RecursiveCopy(boost::filesystem::path("..") / "tests" / "int" / "node" / "stress" / "resources", resourcesPath);

			std::vector<std::string> namespaces{
				"prx",
				"prx.xpx",
				"prx.so",
				"prx.sm",
				"prx.sc",
				"prx.rw",
				"prx.xar",
			};

			std::vector<test::MosaicData> mosaics{
				{ "prx.xpx", Mosaic_Supply, 6, 0, true, false, {} },
				{ "prx.so", Mosaic_Supply, 6, 0, true, true, {} },
				{ "prx.sm", Mosaic_Supply, 6, 0, true, true, {} },
				{ "prx.sc", Mosaic_Supply, 6, 0, true, true, {} },
				{ "prx.rw", Mosaic_Supply, 6, 0, true, true, {} },
				{ "prx.xar", Mosaic_Supply, 6, 0, true, true, {} },
			};

			auto distributionAmount = Amount(Mosaic_Supply / Network_Size);
			auto remainingAmount = Amount(Mosaic_Supply - distributionAmount.unwrap() * Network_Size);
			for (auto& mosaic : mosaics) {
				for (auto i = 0u; i < harvesterKeys.size() - 1; ++i) {
					auto address = extensions::CopyToUnresolvedAddress(model::PublicKeyToAddress(harvesterKeys[i].publicKey(), Network_Identifier));
					mosaic.Distribution.emplace_back(address, distributionAmount);
				}

				auto address = extensions::CopyToUnresolvedAddress(model::PublicKeyToAddress(harvesterKeys[harvesterKeys.size() - 1].publicKey(), Network_Identifier));
				mosaic.Distribution.emplace_back(address, distributionAmount + remainingAmount);
			}

			test::NemesisConfiguration nemesisConfig(
				Network_Identifier,
				test::GenerateRandomByteArray<GenerationHash>(),
				test::GenerateKeyPair(),
				1,
				(seedPath / "data").generic_string(),
				namespaces,
				mosaics,
				harvesterKeys);
			UpdateImmutableConfiguration((seedPath / "resources" / "config-immutable.properties").generic_string(), nemesisConfig);
			UpdateNetworkConfiguration((seedPath / "resources" / "config-network.properties").generic_string(), nemesisConfig.NemesisSignerKeyPair.publicKey(), bootKeys, harvesterKeys);
			test::GenerateAndSaveNemesisBlock(resourcesPath.generic_string(), nemesisConfig);


			auto bootstrapNodes = CreateBootstrapNodes(bootKeys, Bootstrap_Node_Count);
			std::vector<std::unique_ptr<LocalNodeTestContext>> contexts;
			boost::thread_group threads;
			for (auto i = 0u; i < Network_Size; ++i) {
				contexts.push_back(std::make_unique<LocalNodeTestContext>(bootstrapNodes, bootKeys, harvesterKeys, i, seedDir.name()));

				// - boot the nodes
				auto& context = *contexts[i];
				threads.create_thread([&context] {
					context.boot();
					context.localNode().state().setMaxChainHeight(Max_Chain_Height);
				});
			}

			// Act + Assert
			auto success = TryWaitForMaxHeight(contexts, Max_Chain_Height.unwrap() * 10);
			ASSERT_TRUE(success) << "test timed out";

			threads.join_all();

			for (auto i = 0u; i < Network_Size - 1; ++i) {
				auto stats1 = GetStatistics(*contexts[i]);
				auto stats2 = GetStatistics(*contexts[i + 1]);
				ASSERT_EQ(stats1.Score, stats2.Score);
				ASSERT_EQ(stats1.StateHash, stats2.StateHash);
				ASSERT_EQ(stats1.Height, stats2.Height);
			}
		}

		// endregion
	}

	TEST(TEST_CLASS, NetworkBuildsChainSuccess) {
		// Assert:
		AssertNetworkBuildsChainSuccess();
	}
}}
