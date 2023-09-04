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

#pragma once
#include <catapult/cache_core/AccountStateCache.h>
#include "ConfigurationTestUtils.h"
#include "LocalNodeNemesisHashTestUtils.h"
#include "LocalNodeTestUtils.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/local/server/LocalNode.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/StorageTestUtils.h"
#include "tests/test/nemesis/NemesisCompatibleConfiguration.h"
#include "tests/test/nodeps/Filesystem.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "plugins/txes/mosaic/src/config/MosaicConfiguration.h"
#include "plugins/txes/transfer/src/config/TransferConfiguration.h"
#include "plugins/txes/namespace/src/config/NamespaceConfiguration.h"
#include "plugins/services/globalstore/src/config/GlobalStoreConfiguration.h"
#include "plugins/txes/upgrade/src/config/BlockchainUpgradeConfiguration.h"
#include "plugins/txes/lock_fund/src/config/LockFundConfiguration.h"
#include "plugins/txes/config/src/config/NetworkConfigConfiguration.h"

namespace catapult { namespace test {

	/// Traits for a peer node.
	struct LocalNodePeerTraits {
		static constexpr auto CountersToLocalNodeStats = test::CountersToPeerLocalNodeStats;
		static constexpr auto AddPluginExtensions = test::AddPeerPluginExtensions;
	};

	/// Traits for an api node.
	struct LocalNodeApiTraits {
		static constexpr auto CountersToLocalNodeStats = test::CountersToBasicLocalNodeStats;
		static constexpr auto AddPluginExtensions = test::AddApiPluginExtensions;
	};

	/// A common test context for local node tests.
	template<typename TTraits>
	class LocalNodeTestContext {
	public:
		/// Creates a context around \a nodeFlag.
		explicit LocalNodeTestContext(NodeFlag nodeFlag,
									  const std::string& seedDir = "") : LocalNodeTestContext(nodeFlag, std::vector<ionet::Node>{})
		{}

		/// Creates a context around \a nodeFlag with custom \a nodes.
		LocalNodeTestContext(NodeFlag nodeFlag, const std::vector<ionet::Node>& nodes,
							 const std::string& seedDir = "")
				: LocalNodeTestContext(nodeFlag, nodes, [](const auto&) {}, "", seedDir)
		{}
		~LocalNodeTestContext()
		{
			CATAPULT_CLEANUP_LOG(info, "Destroying local node test context");
		}

		/// Creates a context around \a nodeFlag with custom \a nodes, config transform (\a configTransform)
		/// and temp directory postfix (\a tempDirPostfix).
		LocalNodeTestContext(
				NodeFlag nodeFlag,
				const std::vector<ionet::Node>& nodes,
				const consumer<config::BlockchainConfiguration&>& configTransform,
				const std::string& tempDirPostfix,
				const std::string& seedDir= "")
				: m_nodeFlag(nodeFlag)
				, m_nodes(nodes)
				, m_configTransform(configTransform)
				, m_serverKeyPair(loadServerKeyPair())
				, m_partnerServerKeyPair(LoadPartnerServerKeyPair())
				, m_localNodeHarvestingKeys(std::make_tuple(crypto::KeyPair::FromString("819F72066B17FFD71B8B4142C5AEAE4B997B0882ABDF2C263B02869382BD93A0", 1), test::GenerateVrfKeyPair()))
				, m_tempDir("lntc" + tempDirPostfix)
				, m_partnerTempDir("lntc_partner" + tempDirPostfix) {
			if(HasFlag(NodeFlag::Auto_Harvest, nodeFlag)) initializeDataDirectory(m_tempDir.name(), seedDir, &m_localNodeHarvestingKeys);
			else initializeDataDirectory(m_tempDir.name(), seedDir);



			if (HasFlag(NodeFlag::With_Partner, nodeFlag)) {
				initializeDataDirectory(m_partnerTempDir.name(), seedDir);

				// need to call configTransform first so that partner node loads all required transaction plugins
				auto config = CreatePrototypicalBlockchainConfiguration(m_partnerTempDir.name());
				m_configTransform(config);
				auto conf = loadAssociatedNemesisBlock();
				const_cast<model::NetworkConfiguration&>(config.Network) = std::get<0>(conf);
				const_cast<config::SupportedEntityVersions&>(config.SupportedEntityVersions) = std::get<1>(conf);

				// Preload plugin configuraitons for nemesis required plugins

				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::TransferConfiguration>();
				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::MosaicConfiguration>();
				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::NamespaceConfiguration>();
				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::BlockchainUpgradeConfiguration>();
				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::LockFundConfiguration>();
				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::GlobalStoreConfiguration>();
				const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::NetworkConfigConfiguration>();
				m_pLocalPartnerNode = BootLocalPartnerNode(std::move(config), m_partnerServerKeyPair, nodeFlag);
			}

			if (!HasFlag(NodeFlag::Require_Explicit_Boot, nodeFlag))
				boot();
		}

	private:
		void initializeDataDirectory(const std::string& directory, const std::string& seedDir, std::tuple<crypto::KeyPair, crypto::KeyPair>* harvestKeys = nullptr) const {
			PrepareStorage(directory, seedDir);
			PrepareConfiguration(directory, m_nodeFlag, harvestKeys);

			auto config = CreatePrototypicalBlockchainConfiguration(directory);
			prepareNetworkConfiguration(config);

			if (HasFlag(NodeFlag::Verify_Receipts, m_nodeFlag))
				SetNemesisReceiptsHash(directory, config);


			if (HasFlag(NodeFlag::Verify_State, m_nodeFlag))
				SetNemesisStateHash(directory, config);
		}

	public:
		/// Gets the data directory.
		std::string dataDirectory() const {
			return m_tempDir.name();
		}

		/// Gets the resources directory.
		std::string resourcesDirectory() const {
			return m_tempDir.name() + "/resources";
		}

		auto loadAssociatedNemesisBlock() const{
			// load from file storage to allow successive modifications
			io::FileBlockStorage storage(dataDirectory());
			auto pNemesisBlockElement = storage.loadBlockElement(Height(1));

			// modify nemesis block and resign it
			auto& nemesisBlock = const_cast<model::Block&>(pNemesisBlockElement->Block);
			return extensions::NemesisBlockLoader::ReadNetworkConfiguration(pNemesisBlockElement, config::ImmutableConfiguration::Uninitialized());
		}
		/// Gets the primary (first) local node.
		local::LocalNode& localNode() const {
			return *m_pLocalNode;
		}

		/// Gets the node stats.
		auto stats() const {
			return TTraits::CountersToLocalNodeStats(m_pLocalNode->counters());
		}

		/// Loads saved height from persisted state.
		Height loadSavedStateChainHeight() const {
			auto path = boost::filesystem::path(m_tempDir.name()) / "state" / "supplemental.dat";
			io::RawFile file(path.generic_string(), io::OpenMode::Read_Only);
			file.seek(file.size() - sizeof(Height));
			return io::Read<Height>(file);
		}

	public:
		/// Creates a copy of the default blockchain configuration which will be overwritten.
		config::BlockchainConfiguration createConfig() const {
			auto config = CreatePrototypicalBlockchainConfiguration(m_tempDir.name());
			return config;
		}

		/// Prepares a fresh data \a directory and returns corresponding configuration.
		config::BlockchainConfiguration prepareFreshDataDirectory(const std::string& directory, const std::string& seedDir) const {
			initializeDataDirectory(directory, seedDir);
			auto conf = loadAssociatedNemesisBlock();
			auto config = CreatePrototypicalBlockchainConfiguration(directory);
			prepareNetworkConfiguration(config);
			const_cast<model::NetworkConfiguration&>(config.Network) = std::get<0>(conf);
			const_cast<config::SupportedEntityVersions&>(config.SupportedEntityVersions) = std::get<1>(conf);

			// Preload plugin configuraitons for nemesis required plugins

			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::TransferConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::MosaicConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::NamespaceConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::BlockchainUpgradeConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::LockFundConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::GlobalStoreConfiguration>();
			const_cast<model::NetworkConfiguration&>(config.Network).InitPluginConfiguration<config::NetworkConfigConfiguration>();

			return config;
		}

	public:
		/// Boots a new local node.
		/// \note This overload is intended to be called only for nodes that require explicit booting.
		void boot() {
			if (m_pLocalNode)
				CATAPULT_THROW_RUNTIME_ERROR("cannot boot local node multiple times via same test context");

			m_pLocalNode = boot(createConfig());
		}

		/// Get the config holder that was created when the node was booted.
		std::shared_ptr<config::BlockchainConfigurationHolder> configHolder()
		{
			return m_pConfigHolder;
		}

		/// Boots a new local node around \a config.
		std::unique_ptr<local::LocalNode> boot(config::BlockchainConfiguration&& config) {
			return boot(std::move(config), [](const auto&) {});
		}

		/// Boots a new local node allowing additional customization via \a configure and \a cacheModifier.
		std::unique_ptr<local::LocalNode> boot(const consumer<extensions::ProcessBootstrapper&>& configure) {
			return boot(createConfig(), configure);
		}

		/// Resets this context and shuts down the local node.
		void reset() {
			if (m_pLocalPartnerNode)
				CATAPULT_THROW_INVALID_ARGUMENT("local node's partner node is expected to be uninitialized");

			m_pLocalNode->shutdown();
			m_pLocalNode.reset();
		}

	private:

		std::unique_ptr<local::LocalNode> boot(
				config::BlockchainConfiguration&& config,
				const consumer<extensions::ProcessBootstrapper&>& configure) {
			prepareNetworkConfiguration(config);

			m_pConfigHolder = std::make_shared<config::BlockchainConfigurationHolder>(config);

			auto pBootstrapper = std::make_unique<extensions::ProcessBootstrapper>(
					m_pConfigHolder,
					resourcesDirectory(),
					extensions::ProcessDisposition::Production,
					"LocalNodeTests");
			pBootstrapper->addStaticNodes(m_nodes);

			auto& extensionManager = pBootstrapper->extensionManager();
			extensionManager.addServiceRegistrar(std::make_unique<CapturingServiceRegistrar>(m_capturedServiceState));
			pBootstrapper->loadExtensions();
			configure(*pBootstrapper);
			return local::CreateLocalNode(m_serverKeyPair, std::move(pBootstrapper));
		}

	private:
		/// Note: Network configuration is loaded from nemesis block.
		void prepareNetworkConfiguration(config::BlockchainConfiguration& config) const {
			PrepareNetworkConfiguration(config, TTraits::AddPluginExtensions, m_nodeFlag);
			m_configTransform(config);
		}

		crypto::KeyPair loadServerKeyPair() const {
			// can pass empty string to CreateBlockchainConfiguration because this config is only being used to get boot key
			auto config = CreatePrototypicalBlockchainConfiguration("");
			m_configTransform(config);
			return crypto::KeyPair::FromString(config.User.BootKey, Node_Boot_Key_Derivation_Scheme);
		}

	public:
		/// Waits for \a value active readers.
		void waitForNumActiveReaders(size_t value) const {
			WAIT_FOR_VALUE_EXPR(value, stats().NumActiveReaders);
		}

		/// Waits for \a value active writers.
		void waitForNumActiveWriters(size_t value) const {
			WAIT_FOR_VALUE_EXPR(value, stats().NumActiveWriters);
		}

		/// Waits for \a value scheduled tasks.
		void waitForNumScheduledTasks(size_t value) const {
			WAIT_FOR_VALUE_EXPR(value, stats().NumScheduledTasks);
		}

	public:
		/// Gets the captured node subscriber.
		subscribers::NodeSubscriber& nodeSubscriber() const {
			return *m_capturedServiceState.pNodeSubscriber;
		}

	private:
		struct CapturedServiceState {
			subscribers::NodeSubscriber* pNodeSubscriber;
			CATAPULT_DESTRUCTOR_CLEANUP_LOG(info, CapturedServiceState, "Destroying captured service state")
		};

		// service registrar for capturing ServiceState values
		// \note only node subscriber is currently captured
		class CapturingServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit CapturingServiceRegistrar(CapturedServiceState& state) : m_state(state)
			{}

		public:
			extensions::ServiceRegistrarInfo info() const override {
				return { "Capturing", extensions::ServiceRegistrarPhase::Initial };
			}

			void registerServiceCounters(extensions::ServiceLocator&) override {
				// do nothing
			}

			void registerServices(extensions::ServiceLocator&, extensions::ServiceState& state) override {
				m_state.pNodeSubscriber = &state.nodeSubscriber();
			}

		private:
			CapturedServiceState& m_state;
		};

	private:
		NodeFlag m_nodeFlag;
		std::vector<ionet::Node> m_nodes;
		consumer<config::BlockchainConfiguration&> m_configTransform;
		crypto::KeyPair m_serverKeyPair;
		crypto::KeyPair m_partnerServerKeyPair;
		TempDirectoryGuard m_tempDir;
		TempDirectoryGuard m_partnerTempDir;
		std::tuple<crypto::KeyPair, crypto::KeyPair> m_localNodeHarvestingKeys;
		std::unique_ptr<local::LocalNode> m_pLocalPartnerNode;
		std::unique_ptr<local::LocalNode> m_pLocalNode;
		std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
		mutable CapturedServiceState m_capturedServiceState;
	};
}}
