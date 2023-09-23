/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/chain/BlockScorer.h"
#include "catapult/config/ValidateConfiguration.h"
#include "catapult/model/TransactionFeeCalculator.h"
#include "tests/int/node/stress/test/BlockChainBuilder.h"
#include "tests/int/node/stress/test/TransactionsBuilder.h"
#include "tests/int/node/test/LocalNodeRequestTestUtils.h"
#include "tests/int/node/test/LocalNodeTestContext.h"
#include "tests/test/nodeps/Logging.h"
#include "tests/test/nodeps/TestConstants.h"
#include <boost/property_tree/ini_parser.hpp>

namespace pt = boost::property_tree;

namespace catapult { namespace local {

#define TEST_CLASS FastFinalityBlockChainIntegrityTests

	namespace {
		constexpr size_t Network_Size = 10;
		constexpr Height Max_Chain_Height = Height(50);
		constexpr auto Process_Name = "server";

		constexpr const char* Harvesting_Private_Keys[] = {
			"7AA907C3D80B3815BE4B4E1470DEEE8BB83BFEB330B9A82197603D09BA947230",
			"A3B96B1FF2166666662C4F031FFCDC79D97A2A48835A06E5DF8A0CCCE2FE0FB4",
			"81D5258865FA682E1DA52108233A9616BF9252399C1C421CD161CCCF214ACDDD",
			"E5D1AD6B29F900C390C19B822DE88355ED30391CC2684919950AA756D5888431",
			"4DC8F4C8C84ED4A829E79A685A98E4A0BB97A3C6DA9C49FA83CA133676993D08",
			"FD7BFAC074A2F0FA9383C298E1392472FBFE3731E659D46FB094D75EEA609585",
			"819F72066B17FFD71B8B4142C5AEAE4B997B0882ABDF2C263B02869382BD93A0",
			"3E86205091D90B661F95E986D7CC63D68FB11227E8AAF469A5612DB62F89606A",
			"02F15E708EE15E834145048F0251B107A817542E6F288629141E44EF1A188FE8",
			"EDFB348D4AAA333E6D73D9CAD1EA18FE3FE079CC3373E9E4E75A4FBD7D3476E0",
		};

		constexpr const char* Boot_Private_Keys[] = {
			"DF4CCC05F6FF5AEF5D3D1815CBEE8A9DD09B9011C3569018335F131A4EB403D0",
			"83DEA9CFACE42A5288129A56BB2FD949A525B8FA14F5E283D4477E14D1845569",
			"DE62ADE55EA2277358F9CC64F6FD3593083EFB6106EEFE8C93446B0EF10B5C15",
			"4524BFC5428B3546275003AC8EFC80135592F0AC2D99B2C681152A851E2C9433",
			"D96CBF42B5DADF638F4C5A97593D3AE01F1D71E27625D1758DF3C8389EE04D4B",
			"CA37960E62577AB26A457A1C6B7F30F286B18AA1EA03FE64F14D2F4BC3A5E9AE",
			"25B25DCDA948A0CFC5A7FB4607DA02733D1FDFB16DE75FCADFC77BC01A726056",
			"F119A46881F70FD57990FD1F5ADAF00294F53EDC843F957D77C44B3BB034EA82",
			"E8DAFF4218CC6E3185ABE506D084DCF387A1C9986F9ED9244D72110A7998FCCF",
			"38F9AF621960A620DB4E6FFAA833080CBD85F94067F49A8E0EDEC030EBF6D70C",
		};

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
//			return static_cast<uint16_t>(test::GetLocalHostPort() + 10 * (id + 1));
			return static_cast<uint16_t>(7900 + 10 * id);
		}

		void SetAutoHarvesting(const std::string& configFilePath, const char* harvestKey) {
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
			pt::write_ini(configFilePath, properties);
		}

		void SetUserProperties(const std::string& configFilePath, uint32_t id, const std::string& dataPath) {
			pt::ptree properties;
			pt::read_ini(configFilePath, properties);
			properties.put("account.bootKey", Boot_Private_Keys[id]);
			properties.put("storage.dataDirectory", dataPath);
			pt::write_ini(configFilePath, properties);
		}

		void PrepareConfiguration(const std::string& destination, uint32_t id) {
			auto sourcePath = boost::filesystem::path("..") / "tests" / "int" / "node" / "stress";
			auto destinationPath = boost::filesystem::path(destination);
			auto dataPath = destinationPath / "data";
			auto resourcesPath = destinationPath / "resources";

			RecursiveCopy(sourcePath / "data", destinationPath / "data");
			RecursiveCopy(sourcePath / "resources", destinationPath / "resources");

			auto harvestingConfigPath = resourcesPath / "config-harvesting.properties";
			SetAutoHarvesting(harvestingConfigPath.generic_string(), Harvesting_Private_Keys[id]);

			auto nodeConfigPath = resourcesPath / "config-node.properties";
			SetNodeProperties(nodeConfigPath.generic_string(), id);

			auto userConfigPath = resourcesPath / "config-user.properties";
			SetUserProperties(userConfigPath.generic_string(), id, dataPath.generic_string());
		}

		class LocalNodeTestContext {
		public:
			LocalNodeTestContext(
					const std::vector<ionet::Node>& nodes,
					uint32_t id)
				: m_nodes(nodes)
				, m_serverKeyPair(crypto::KeyPair::FromString(Boot_Private_Keys[id]))
				, m_tempDir("lntc_" + std::to_string(id))
				, m_id(id) {
			}

		private:
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

		public:
			/// Boots a new local node.
			void boot() {
				if (m_pLocalNode)
					CATAPULT_THROW_RUNTIME_ERROR("cannot boot local node multiple times via same test context");

				PrepareConfiguration(dataDirectory(), m_id);

				auto config = config::BlockchainConfiguration::LoadFromPath(resourcesDirectory(), Process_Name);
				auto pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);

				auto pBootstrapper = std::make_unique<extensions::ProcessBootstrapper>(
					pConfigHolder,
					resourcesDirectory(),
					extensions::ProcessDisposition::Production,
					"LocalNodeTests");
				pBootstrapper->addStaticNodes(m_nodes);
				pBootstrapper->loadExtensions();

				m_pLocalNode = local::CreateLocalNode(m_serverKeyPair, std::move(pBootstrapper));
			}

			/// Resets this context and shuts down the local node.
			void reset() {
				m_pLocalNode->shutdown();
				m_pLocalNode.reset();
			}

		private:
			std::vector<ionet::Node> m_nodes;
			crypto::KeyPair m_serverKeyPair;
			test::TempDirectoryGuard m_tempDir;
			uint32_t m_id;
			std::unique_ptr<local::LocalNode> m_pLocalNode;
		};

		ionet::Node CreateNode(uint32_t id) {
			auto metadata = ionet::NodeMetadata(model::NetworkIdentifier::Mijin_Test, "NODE " + std::to_string(id));
			metadata.Roles = ionet::NodeRoles::Peer;
			return {
				crypto::KeyPair::FromString(Boot_Private_Keys[id]).publicKey(),
				test::CreateLocalHostNodeEndpoint(GetPortForNode(id)),
				metadata
			};
		}

		std::vector<ionet::Node> CreateNodes() {
			std::vector<ionet::Node> nodes;
			for (auto i = 0u; i < Network_Size; ++i)
				nodes.push_back(CreateNode(i));

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
				auto storageView = pContext->localNode().state().storage().view();
				if (storageView.chainHeight() < Max_Chain_Height)
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

		void AssertNetworkBuildsChainSuccess() {
			// Arrange: create nodes
			auto networkNodes = CreateNodes();

			std::vector<std::unique_ptr<LocalNodeTestContext>> contexts;
			for (auto i = 0u; i < Network_Size; ++i) {
				contexts.push_back(std::make_unique<LocalNodeTestContext>(networkNodes, i));

				// - boot the node
				auto& context = *contexts[i];
				context.boot();
				context.localNode().state().setMaxChainHeight(Max_Chain_Height);
			}

			auto begin = std::chrono::high_resolution_clock::now();

			auto success = TryWaitForMaxHeight(contexts, Max_Chain_Height.unwrap() * 10);
			ASSERT_TRUE(success) << "test timed out";

			auto current = std::chrono::high_resolution_clock::now();
			auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(current - begin).count();
			CATAPULT_LOG(error) << "=======================================> TEST TIME = " << elapsedSeconds << " seconds";

			for (auto i = 0u; i < Network_Size - 1; ++i) {
				auto stats1 = GetStatistics(*contexts[i]);
				auto stats2 = GetStatistics(*contexts[i]);
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
