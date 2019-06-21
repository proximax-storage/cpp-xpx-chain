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

#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/plugins/PluginExceptions.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/MutableCatapultConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS ProcessBootstrapperTests

	// region basic

	TEST(TEST_CLASS, CanCreateBootstrapper) {
		// Arrange:
		auto config = test::CreateUninitializedCatapultConfiguration();
		const_cast<uint32_t&>(config.BlockChain.BlockPruneInterval) = 15;
		const_cast<bool&>(config.Node.ShouldUseCacheDatabaseStorage) = true;
		const_cast<std::string&>(config.User.DataDirectory) = "base_data_dir";

		// Act:
		ProcessBootstrapper bootstrapper(config, "resources path", ProcessDisposition::Recovery, "bootstrapper");

		// Assert: compare BlockPruneInterval as a sentinel value because the bootstrapper copies the config
		EXPECT_EQ(15u, bootstrapper.config().BlockChain.BlockPruneInterval);

		const auto& pluginManager = bootstrapper.pluginManager();
		EXPECT_EQ(15u, pluginManager.config().BlockPruneInterval);
		EXPECT_TRUE(pluginManager.storageConfig().PreferCacheDatabase);
		EXPECT_EQ("base_data_dir/statedb", pluginManager.storageConfig().CacheDatabaseDirectory);

		// - resources path and disposition should be correct
		EXPECT_EQ("resources path", bootstrapper.resourcesPath());
		EXPECT_EQ(ProcessDisposition::Recovery, bootstrapper.disposition());

		// - nodes should be empty
		EXPECT_TRUE(bootstrapper.staticNodes().empty());

		// - pool should have default number of threads
		EXPECT_EQ(std::thread::hardware_concurrency(), bootstrapper.pool().numWorkerThreads());

		// - other managers should not throw
		bootstrapper.extensionManager();
		bootstrapper.subscriptionManager();
	}

	// endregion

	// region loadExtensions

	namespace {
		ProcessBootstrapper CreateBootstrapper(const config::CatapultConfiguration& config) {
			return ProcessBootstrapper(config, "resources path", ProcessDisposition::Production, "bootstrapper");
		}

		template<typename TAction>
		void RunExtensionsTest(const std::string& directory, const std::string& name, TAction action) {
			// Arrange:
			test::MutableCatapultConfiguration config;
			config.User.PluginsDirectory = directory;
			config.Extensions.Names = { name };
			auto bootstrapper = CreateBootstrapper(config.ToConst());

			// Act + Assert:
			action(bootstrapper);
		}

		void AssertCanLoadKnownDynamicallyLinkedPlugins(const std::string& directory) {
			// Arrange:
			RunExtensionsTest(directory, "extension.hashcache", [](auto& bootstrapper) {
				// Sanity:
				const auto& systemPluginNames = bootstrapper.extensionManager().systemPluginNames();
				EXPECT_EQ(2u, systemPluginNames.size());

				// Act:
				bootstrapper.loadExtensions();

				// Assert: hash cache plugin registers a single system plugin
				EXPECT_EQ(3u, systemPluginNames.size());
			});
		}

		void AssertCannotLoadUnknownPlugin(const std::string& directory) {
			// Arrange:
			RunExtensionsTest(directory, "extension.awesome", [](auto& bootstrapper) {
				// Act + Assert:
				EXPECT_THROW(bootstrapper.loadExtensions(), catapult_invalid_argument);
			});
		}
	}

	TEST(TEST_CLASS, CanLoadKnownDynamicallyLinkedPlugins_ExplicitDirectory) {
		// Assert:
		AssertCanLoadKnownDynamicallyLinkedPlugins(test::GetExplicitPluginsDirectory());
	}

	TEST(TEST_CLASS, CanLoadKnownDynamicallyLinkedPlugins_ImplicitDirectory) {
		// Assert:
		AssertCanLoadKnownDynamicallyLinkedPlugins("");
	}

	TEST(TEST_CLASS, CannotLoadUnknownPlugin_ExplicitDirectory) {
		// Assert:
		AssertCannotLoadUnknownPlugin(test::GetExplicitPluginsDirectory());
	}

	TEST(TEST_CLASS, CannotLoadUnknownPlugin_ImplicitDirectory) {
		// Assert:
		AssertCannotLoadUnknownPlugin("");
	}

	TEST(TEST_CLASS, PluginRegistrationExceptionIsForwarded) {
		// Arrange:
		bool isExceptionHandled = false;
		try {
			// - create the bootstrapper
			RunExtensionsTest("", "extension.harvesting", [](auto& bootstrapper) {
				// Act: force an exception by loading harvesting extension with improper resources path
				bootstrapper.loadExtensions();
			});
		} catch (const plugins::plugin_load_error&) {
			CATAPULT_LOG(debug) << UNHANDLED_EXCEPTION_MESSAGE("while running test");
			isExceptionHandled = true;
		}

		// Assert:
		EXPECT_TRUE(isExceptionHandled);
	}

	// endregion

	// region staticNodes + AddStaticNodesFromPath

	TEST(TEST_CLASS, CanAddStaticNodes) {
		// Arrange:
		auto bootstrapper = CreateBootstrapper(test::CreateUninitializedCatapultConfiguration());

		// - add five nodes
		std::vector<ionet::Node> nodes1{
			test::CreateLocalHostNode(test::GenerateRandomByteArray<Key>()),
			test::CreateLocalHostNode(test::GenerateRandomByteArray<Key>())
		};

		std::vector<ionet::Node> nodes2{
			test::CreateLocalHostNode(test::GenerateRandomByteArray<Key>()),
			test::CreateLocalHostNode(test::GenerateRandomByteArray<Key>()),
			test::CreateLocalHostNode(test::GenerateRandomByteArray<Key>())
		};

		// Act:
		bootstrapper.addStaticNodes(nodes1);
		bootstrapper.addStaticNodes(nodes2);
		const auto& bootstrapperNodes = bootstrapper.staticNodes();

		// Assert:
		ASSERT_EQ(5u, bootstrapperNodes.size());

		for (auto i = 0u; i < nodes1.size(); ++i)
			EXPECT_EQ(bootstrapperNodes[i], nodes1[i]) << "nodes1 at " << i;

		for (auto i = 0u; i < nodes2.size(); ++i)
			EXPECT_EQ(bootstrapperNodes[nodes1.size() + i], nodes2[i]) << "nodes2 at " << i;
	}

	TEST(TEST_CLASS, CanAddStaticNodesFromPath) {
		// Arrange:
		auto bootstrapper = CreateBootstrapper(test::CreateUninitializedCatapultConfiguration());

		// Act:
		AddStaticNodesFromPath(bootstrapper, "../resources/peers-p2p.json");

		// Assert:
		EXPECT_EQ(1u, bootstrapper.staticNodes().size());
	}

	// endregion
}}
