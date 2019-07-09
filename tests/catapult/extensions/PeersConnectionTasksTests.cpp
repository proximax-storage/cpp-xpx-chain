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

#include "catapult/extensions/PeersConnectionTasks.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/ionet/NodeInteractionResult.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/net/NodeTestUtils.h"
#include "tests/test/net/mocks/MockPacketWriters.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS PeersConnectionTasksTests

	// region utils

	namespace {
		config::NodeConfiguration::ConnectionsSubConfiguration CreateConfiguration() {
			return { 5, 8, 250, 2 };
		}

		void Add(ionet::NodeContainer& container, const Key& identityKey, const std::string& nodeName, ionet::NodeRoles roles) {
			container.modifier().add(test::CreateNamedNode(identityKey, nodeName, roles), ionet::NodeSource::Dynamic);
		}
	}

	// endregion

	// region CreateNodeAger

	TEST(TEST_CLASS, NodeAger_AgesMatchingConnections) {
		// Arrange:
		auto serviceId = ionet::ServiceIdentifier(123);
		ionet::NodeContainer container;
		auto keys = test::GenerateRandomDataVector<Key>(3);
		Add(container, keys[0], "bob", ionet::NodeRoles::Peer);
		Add(container, keys[1], "alice", ionet::NodeRoles::Peer);
		Add(container, keys[2], "charlie", ionet::NodeRoles::Peer);
		{
			auto modifier = container.modifier();
			modifier.provisionConnectionState(serviceId, keys[0]).Age = 1;
			modifier.provisionConnectionState(serviceId, keys[1]).Age = 2;
			modifier.provisionConnectionState(serviceId, keys[2]).Age = 3;
		}

		// Act:
		auto ager = CreateNodeAger(serviceId, CreateConfiguration(), container);
		ager({ keys[0], keys[2] });

		// Assert: nodes { 0, 2 } are aged, node { 1 } is cleared
		auto view = container.view();
		EXPECT_EQ(2u, view.getNodeInfo(keys[0]).getConnectionState(serviceId)->Age);
		EXPECT_EQ(0u, view.getNodeInfo(keys[1]).getConnectionState(serviceId)->Age);
		EXPECT_EQ(4u, view.getNodeInfo(keys[2]).getConnectionState(serviceId)->Age);
	}

	TEST(TEST_CLASS, NodeAger_AgesMatchingConnectionBans) {
		// Arrange:
		auto serviceId = ionet::ServiceIdentifier(123);
		ionet::NodeContainer container;
		auto keys = test::GenerateRandomDataVector<Key>(3);
		Add(container, keys[0], "bob", ionet::NodeRoles::Peer);
		Add(container, keys[1], "alice", ionet::NodeRoles::Peer);
		Add(container, keys[2], "charlie", ionet::NodeRoles::Peer);
		{
			auto modifier = container.modifier();
			modifier.provisionConnectionState(serviceId, keys[0]).NumConsecutiveFailures = 3;
			modifier.provisionConnectionState(serviceId, keys[0]).BanAge = 11;
			modifier.provisionConnectionState(serviceId, keys[1]).NumConsecutiveFailures = 3;
			modifier.provisionConnectionState(serviceId, keys[1]).BanAge = 100;
			modifier.provisionConnectionState(serviceId, keys[2]).NumConsecutiveFailures = 3;
		}

		auto config = CreateConfiguration();
		config.MaxConnectionBanAge = 100;
		config.NumConsecutiveFailuresBeforeBanning = 3;

		// Act:
		auto ager = CreateNodeAger(serviceId, config, container);
		ager({});

		// Assert: all banned nodes with matching service are aged irrespective of identities passed to ager
		auto view = container.view();
		EXPECT_EQ(12u, view.getNodeInfo(keys[0]).getConnectionState(serviceId)->BanAge);
		EXPECT_EQ(0u, view.getNodeInfo(keys[1]).getConnectionState(serviceId)->BanAge);
		EXPECT_EQ(1u, view.getNodeInfo(keys[2]).getConnectionState(serviceId)->BanAge);
	}

	// endregion

	// region SelectorSettings

	namespace {
		void AssertImportanceDescriptor(
				const SelectorSettings& settings,
				const Key& key,
				Importance expectedImportance,
				Importance expectedTotalChainImportance,
				const std::string& message) {
			const auto& descriptor = settings.ImportanceRetriever(key);
			EXPECT_EQ(expectedImportance, descriptor.Importance) << message;
			EXPECT_EQ(expectedTotalChainImportance, descriptor.TotalChainImportance) << message;
		}

		template<typename TSettingsFactory>
		void AssertCanCreateSelectorSettings(ionet::NodeRoles expectedRole, TSettingsFactory settingsFactory) {
			// Arrange:
			auto serviceState = test::ServiceTestState();
			auto unknownKey = test::GenerateRandomData<Key_Size>();
			auto knownKeyWrongHeight = test::GenerateRandomData<Key_Size>();
			auto knownKey = test::GenerateRandomData<Key_Size>();

			// -  initialize a cache
			auto& blockChainConfig = const_cast<model::BlockChainConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).BlockChain);
			blockChainConfig.ImportanceGrouping = 1;
			blockChainConfig.TotalChainImportance = Importance(100);
			auto& nodeConfig = const_cast<config::NodeConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).Node);
			nodeConfig.OutgoingConnections = CreateConfiguration();
			{
				auto cacheDelta = serviceState.state().cache().createDelta();
				auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(knownKeyWrongHeight, Height(1));
				auto& wrongHeightAccount = accountStateCacheDelta.find(knownKeyWrongHeight).get();
				wrongHeightAccount.Balances.track(test::Default_Currency_Mosaic_Id);
				wrongHeightAccount.Balances.credit(test::Default_Currency_Mosaic_Id, Amount(222), Height(1001));

				accountStateCacheDelta.addAccount(knownKey, Height(1));
				auto& knownKeyAccount = accountStateCacheDelta.find(knownKey).get();
				knownKeyAccount.Balances.track(test::Default_Currency_Mosaic_Id);
				knownKeyAccount.Balances.credit(test::Default_Currency_Mosaic_Id, Amount(111), Height(999));
				serviceState.state().cache().commit(Height(1000));
			}

			// Act:
			auto settings = settingsFactory(serviceState.state(), ionet::ServiceIdentifier(4));

			// Assert:
			EXPECT_EQ(&serviceState.state().nodes(), &settings.Nodes);
			EXPECT_EQ(ionet::ServiceIdentifier(4), settings.ServiceId);
			EXPECT_EQ(expectedRole, settings.RequiredRole);
			EXPECT_EQ(5u, settings.Config.MaxConnections); // only check one config field as proxy

			AssertImportanceDescriptor(settings, unknownKey, Importance(0), Importance(100), "unknownKey");
			AssertImportanceDescriptor(settings, knownKeyWrongHeight, Importance(0), Importance(100), "knownKeyWrongHeight");
			AssertImportanceDescriptor(settings, knownKey, Importance(111), Importance(100), "knownKey");
		}
	}

	TEST(TEST_CLASS, CanCreateSelectorSettingsWithRole) {
		// Assert:
		AssertCanCreateSelectorSettings(ionet::NodeRoles::Api, [](
			auto& state,
			auto serviceId) {
			return SelectorSettings(state, serviceId, ionet::NodeRoles::Api);
		});
	}

	TEST(TEST_CLASS, CanCreateSelectorSettingsWithoutRole) {
		// Assert:
		AssertCanCreateSelectorSettings(ionet::NodeRoles::None, [](
				auto& state,
				auto serviceId) {
			return SelectorSettings(state, serviceId);
		});
	}

	// endregion

	// region CreateNodeSelector

	TEST(TEST_CLASS, CanCreateNodeSelector) {
		// Arrange:
		auto serviceState = test::ServiceTestState();
		auto& blockChainConfig = const_cast<model::BlockChainConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).BlockChain);
		blockChainConfig.ImportanceGrouping = 1;
		blockChainConfig.TotalChainImportance = Importance(100);
		auto& nodeConfig = const_cast<config::NodeConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).Node);
		nodeConfig.OutgoingConnections = CreateConfiguration();
		auto serviceId = ionet::ServiceIdentifier(1);
		auto settings = SelectorSettings(serviceState.state(), serviceId, ionet::NodeRoles::Api);
		auto selector = CreateNodeSelector(settings);

		// Act:
		auto result = selector();

		// Assert:
		EXPECT_TRUE(result.AddCandidates.empty());
		EXPECT_TRUE(result.RemoveCandidates.empty());
	}

	TEST(TEST_CLASS, CreateNodeSelectorProvisionsConnectionStatesForRoleCompatibleNodes) {
		// Arrange:
		auto serviceState = test::ServiceTestState();
		auto keys = test::GenerateRandomDataVector<Key>(3);
		Add(serviceState.state().nodes(), keys[0], "bob", ionet::NodeRoles::Api);
		Add(serviceState.state().nodes(), keys[1], "alice", ionet::NodeRoles::Peer);
		Add(serviceState.state().nodes(), keys[2], "charlie", ionet::NodeRoles::Api | ionet::NodeRoles::Peer);
		auto& blockChainConfig = const_cast<model::BlockChainConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).BlockChain);
		blockChainConfig.ImportanceGrouping = 1;
		blockChainConfig.TotalChainImportance = Importance(100);
		auto& nodeConfig = const_cast<config::NodeConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).Node);
		nodeConfig.OutgoingConnections = CreateConfiguration();
		auto serviceId = ionet::ServiceIdentifier(1);
		auto settings = SelectorSettings(serviceState.state(), serviceId, ionet::NodeRoles::Api);

		// Act:
		CreateNodeSelector(settings);

		// Assert:
		const auto& view = serviceState.state().nodes().view();
		EXPECT_TRUE(!!view.getNodeInfo(keys[0]).getConnectionState(serviceId));
		EXPECT_FALSE(!!view.getNodeInfo(keys[1]).getConnectionState(serviceId));
		EXPECT_TRUE(!!view.getNodeInfo(keys[2]).getConnectionState(serviceId));
	}

	// endregion

	// region ConnectPeersTask: utils

	namespace {
		std::vector<ionet::Node> SeedAlternatingServiceNodes(
				test::ServiceTestState& serviceState,
				uint32_t numNodes,
				ionet::ServiceIdentifier evenServiceId,
				ionet::ServiceIdentifier oddServiceId) {
			std::vector<ionet::Node> nodes;
			auto modifier = serviceState.state().nodes().modifier();
			for (auto i = 0u; i < numNodes; ++i) {
				auto identityKey = test::GenerateRandomData<Key_Size>();
				auto node = test::CreateNamedNode(identityKey, "node " + std::to_string(i));
				modifier.add(node, ionet::NodeSource::Dynamic);
				nodes.push_back(node);
				test::AddNodeInteractions(modifier, identityKey, 7, 3);

				auto serviceId = 0 == i % 2 ? evenServiceId : oddServiceId;
				auto& connectionState = modifier.provisionConnectionState(serviceId, identityKey);
				connectionState.Age = i + 1;
				connectionState.NumConsecutiveFailures = 1;
				connectionState.BanAge = 123;
			}

			return nodes;
		}

		void ConnectSyncAll(mocks::MockPacketWriters& writers, const ionet::NodeSet& nodes) {
			for (const auto& node : nodes)
				writers.connectSync(node);
		}

		void RunConnectPeersTask(
				test::ServiceTestState& serviceState,
				net::PacketWriters& packetWriters,
				ionet::ServiceIdentifier serviceId,
				const NodeSelector& selector = NodeSelector()) {
			// Act:
			auto& blockChainConfig = const_cast<model::BlockChainConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).BlockChain);
			blockChainConfig.ImportanceGrouping = 1;
			blockChainConfig.TotalChainImportance = Importance(100);
			auto& nodeConfig = const_cast<config::NodeConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).Node);
			nodeConfig.OutgoingConnections = CreateConfiguration();
			auto settings = SelectorSettings(serviceState.state(), serviceId, ionet::NodeRoles::Peer);
			auto task = selector
					? CreateConnectPeersTask(settings, packetWriters, selector)
					: CreateConnectPeersTask(settings, packetWriters);
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ("connect peers task", task.Name);
			EXPECT_EQ(thread::TaskResult::Continue, result);
		}
	}

	// endregion

	// region ConnectPeersTask: connection aging

	namespace {
		template<typename TRunTask>
		void AssertMatchingServiceNodesAreAged(TRunTask runTask) {
			// Arrange: prepare a container with alternating matching service nodes
			auto serviceId = ionet::ServiceIdentifier(3);
			auto serviceState = test::ServiceTestState();
			auto nodes = SeedAlternatingServiceNodes(serviceState, 10, ionet::ServiceIdentifier(9), serviceId);

			// - indicate 3 / 5 matching service nodes are active
			mocks::MockPacketWriters writers;
			ConnectSyncAll(writers, { nodes[1], nodes[5], nodes[7] });

			// Act: run selection that returns neither add nor remove candidates
			runTask(serviceState, writers, serviceId);

			// Assert: nodes associated with service 3 were aged, others were untouched
			auto i = 0u;
			auto view = serviceState.state().nodes().view();
			std::vector<uint32_t> expectedAges{ 3, 0, 7, 9, 0 }; // only for matching service 3
			for (const auto& node : nodes) {
				auto message = "node at " + std::to_string(i);
				const auto& nodeInfo = view.getNodeInfo(node.identityKey());
				const auto* pConnectionState = nodeInfo.getConnectionState(serviceId);

				if (0 == i % 2) {
					EXPECT_FALSE(!!pConnectionState) << message;
				} else {
					ASSERT_TRUE(!!pConnectionState) << message;
					EXPECT_EQ(expectedAges[i / 2], pConnectionState->Age) << message;
				}

				++i;
			}
		}
	}

	TEST(TEST_CLASS, ConnectPeersTask_MatchingServiceNodesAreAged) {
		// Assert:
		AssertMatchingServiceNodesAreAged([](auto& serviceState, auto& writers, auto serviceId) {
			RunConnectPeersTask(serviceState, writers, serviceId);
		});
	}

	// endregion

	// region ConnectPeersTask: remove candidates

	namespace {
		template<typename TRunTask>
		void AssertRemoveCandidatesAreClosedInWriters(TRunTask runTask) {
			// Arrange: prepare a container with alternating matching service nodes
			auto serviceId = ionet::ServiceIdentifier(3);
			auto serviceState = test::ServiceTestState();
			auto nodes = SeedAlternatingServiceNodes(serviceState, 10, ionet::ServiceIdentifier(9), serviceId);

			// - indicate 3 / 5 matching service nodes are active
			mocks::MockPacketWriters writers;
			ConnectSyncAll(writers, { nodes[1], nodes[5], nodes[7] });

			// Act: run selection that returns remove candidates
			auto removeCandidates = utils::KeySet{ nodes[1].identityKey(), nodes[9].identityKey() };
			runTask(serviceState, writers, serviceId, removeCandidates);

			// Assert: remove candidates were removed from writers
			EXPECT_EQ(removeCandidates, writers.closedNodeIdentities());

			// - removed nodes are still aged if they are active during selection (their ages will be zeroed on next iteration)
			auto view = serviceState.state().nodes().view();
			EXPECT_EQ(3u, view.getNodeInfo(nodes[1].identityKey()).getConnectionState(serviceId)->Age);
			EXPECT_EQ(0u, view.getNodeInfo(nodes[9].identityKey()).getConnectionState(serviceId)->Age);
		}
	}

	TEST(TEST_CLASS, ConnectPeersTask_RemoveCandidatesAreClosedInWriters) {
		// Assert:
		AssertRemoveCandidatesAreClosedInWriters([](auto& serviceState, auto& writers, auto serviceId, const auto& removeCandidates) {
			RunConnectPeersTask(serviceState, writers, serviceId, [&removeCandidates]() {
				auto result = NodeSelectionResult();
				result.RemoveCandidates = removeCandidates;
				return result;
			});
		});
	}

	// endregion

	// region ConnectPeersTask: add candidates

	namespace {
		bool IsConnectedNode(const mocks::MockPacketWriters& writers, const ionet::Node& searchNode) {
			const auto& nodes = writers.connectedNodes();
			return std::any_of(nodes.cbegin(), nodes.cend(), [searchNode](const auto& node) {
				return searchNode == node;
			});
		}

		void AssertInteractions(
				const ionet::NodeContainerView& view,
				const Key& identityKey,
				uint32_t expectedNumSuccesses,
				uint32_t expectedNumFailures) {
			auto interactions = view.getNodeInfo(identityKey).interactions(Timestamp());
			test::AssertNodeInteractions(expectedNumSuccesses, expectedNumFailures, interactions);
		}

		void AssertConnectionState(
				const ionet::NodeContainerView& view,
				const Key& identityKey,
				ionet::ServiceIdentifier serviceId,
				bool hasLastSuccess) {
			const auto& nodeInfo = view.getNodeInfo(identityKey);
			const auto& connectionState = *nodeInfo.getConnectionState(serviceId);

			if (hasLastSuccess) {
				EXPECT_EQ(0u, connectionState.NumConsecutiveFailures);
				EXPECT_EQ(0u, connectionState.BanAge);
			} else {
				EXPECT_EQ(3u, connectionState.NumConsecutiveFailures); // should be incremented (initial value 2)
				EXPECT_EQ(124u, connectionState.BanAge); // should be incremented (initial value 123)
			}
		}

		void IncrementNumConsecutiveFailures(ionet::NodeContainer& container, ionet::ServiceIdentifier serviceId, const Key& identityKey) {
			++container.modifier().provisionConnectionState(serviceId, identityKey).NumConsecutiveFailures;
		}
	}

	TEST(TEST_CLASS, ConnectPeersTask_AddCandidatesHaveConnectionsInitiatedAndInteractionsUpdated_AllSucceed) {
		// Arrange: prepare a container with alternating matching service nodes
		auto serviceId = ionet::ServiceIdentifier(3);
		auto serviceState = test::ServiceTestState();
		auto nodes = SeedAlternatingServiceNodes(serviceState, 12, ionet::ServiceIdentifier(9), serviceId);

		mocks::MockPacketWriters writers;

		// Act: run selection that returns add candidates
		auto addCandidates = ionet::NodeSet{ nodes[3], nodes[5], nodes[9] };
		RunConnectPeersTask(serviceState, writers, serviceId, [&addCandidates]() {
			auto result = NodeSelectionResult();
			result.AddCandidates = addCandidates;
			return result;
		});

		// Assert: add candidates were added to writers
		EXPECT_TRUE(IsConnectedNode(writers, nodes[3]));
		EXPECT_TRUE(IsConnectedNode(writers, nodes[5]));
		EXPECT_TRUE(IsConnectedNode(writers, nodes[9]));

		// - interactions have been updated appropriately from initial values (7S, 3F)
		auto view = serviceState.state().nodes().view();
		AssertInteractions(view, nodes[3].identityKey(), 8, 3);
		AssertInteractions(view, nodes[5].identityKey(), 8, 3);
		AssertInteractions(view, nodes[9].identityKey(), 8, 3);

		// - connection states have correct number of consecutive failures and ban age
		AssertConnectionState(view, nodes[3].identityKey(), serviceId, true);
		AssertConnectionState(view, nodes[5].identityKey(), serviceId, true);
		AssertConnectionState(view, nodes[9].identityKey(), serviceId, true);
	}

	TEST(TEST_CLASS, ConnectPeersTask_AddCandidatesHaveConnectionsInitiatedAndInteractionsUpdated_SomeSucceed) {
		// Arrange: prepare a container with alternating matching service nodes and increment consecutive failures for some nodes
		//          so that those nodes have requisite number of consecutive failures for banning
		auto serviceId = ionet::ServiceIdentifier(3);
		auto serviceState = test::ServiceTestState();
		auto nodes = SeedAlternatingServiceNodes(serviceState, 12, ionet::ServiceIdentifier(9), serviceId);
		IncrementNumConsecutiveFailures(serviceState.state().nodes(), serviceId, nodes[3].identityKey());
		IncrementNumConsecutiveFailures(serviceState.state().nodes(), serviceId, nodes[9].identityKey());

		// - trigger some nodes to fail during connection
		mocks::MockPacketWriters writers;
		writers.setConnectCode(nodes[3].identityKey(), net::PeerConnectCode::Socket_Error);
		writers.setConnectCode(nodes[9].identityKey(), net::PeerConnectCode::Socket_Error);

		// Act: run selection that returns add candidates
		auto addCandidates = ionet::NodeSet{ nodes[3], nodes[5], nodes[9] };
		RunConnectPeersTask(serviceState, writers, serviceId, [&addCandidates]() {
			auto result = NodeSelectionResult();
			result.AddCandidates = addCandidates;
			return result;
		});

		// Assert: add candidates were added to writers
		EXPECT_TRUE(IsConnectedNode(writers, nodes[3]));
		EXPECT_TRUE(IsConnectedNode(writers, nodes[5]));
		EXPECT_TRUE(IsConnectedNode(writers, nodes[9]));

		// - interactions have been updated appropriately from initial values (7S, 3F)
		auto view = serviceState.state().nodes().view();
		AssertInteractions(view, nodes[3].identityKey(), 7, 4);
		AssertInteractions(view, nodes[5].identityKey(), 8, 3);
		AssertInteractions(view, nodes[9].identityKey(), 7, 4);

		// - connection states have correct number of consecutive failures and ban age
		AssertConnectionState(view, nodes[3].identityKey(), serviceId, false);
		AssertConnectionState(view, nodes[5].identityKey(), serviceId, true);
		AssertConnectionState(view, nodes[9].identityKey(), serviceId, false);
	}

	TEST(TEST_CLASS, ConnectPeersTask_AddCandidatesHaveConnectionsInitiatedAndInteractionsUpdated_NoneSucceed) {
		// Arrange: prepare a container with alternating matching service nodes and increment consecutive failures fo all nodes
		//          so that those nodes have requisite number of consecutive failures for banning
		auto serviceId = ionet::ServiceIdentifier(3);
		auto serviceState = test::ServiceTestState();
		auto nodes = SeedAlternatingServiceNodes(serviceState, 12, ionet::ServiceIdentifier(9), serviceId);
		IncrementNumConsecutiveFailures(serviceState.state().nodes(), serviceId, nodes[3].identityKey());
		IncrementNumConsecutiveFailures(serviceState.state().nodes(), serviceId, nodes[5].identityKey());
		IncrementNumConsecutiveFailures(serviceState.state().nodes(), serviceId, nodes[9].identityKey());

		// - trigger all nodes to fail during connection
		mocks::MockPacketWriters writers;
		writers.setConnectCode(nodes[3].identityKey(), net::PeerConnectCode::Socket_Error);
		writers.setConnectCode(nodes[5].identityKey(), net::PeerConnectCode::Socket_Error);
		writers.setConnectCode(nodes[9].identityKey(), net::PeerConnectCode::Socket_Error);

		// Act: run selection that returns add candidates
		auto addCandidates = ionet::NodeSet{ nodes[3], nodes[5], nodes[9] };
		RunConnectPeersTask(serviceState, writers, serviceId, [&addCandidates]() {
			auto result = NodeSelectionResult();
			result.AddCandidates = addCandidates;
			return result;
		});

		// Assert: add candidates were added to writers
		EXPECT_TRUE(IsConnectedNode(writers, nodes[3]));
		EXPECT_TRUE(IsConnectedNode(writers, nodes[5]));
		EXPECT_TRUE(IsConnectedNode(writers, nodes[9]));

		// - interactions have been updated appropriately from initial values (7S, 3F)
		auto view = serviceState.state().nodes().view();
		AssertInteractions(view, nodes[3].identityKey(), 7, 4);
		AssertInteractions(view, nodes[5].identityKey(), 7, 4);
		AssertInteractions(view, nodes[9].identityKey(), 7, 4);

		// - connection states have correct number of consecutive failures and ban age
		AssertConnectionState(view, nodes[3].identityKey(), serviceId, false);
		AssertConnectionState(view, nodes[5].identityKey(), serviceId, false);
		AssertConnectionState(view, nodes[9].identityKey(), serviceId, false);
	}

	// endregion

	// region CreateRemoveOnlyNodeSelector

	TEST(TEST_CLASS, CanCreateRemoveOnlyNodeSelector) {
		// Arrange:
		auto serviceState = test::ServiceTestState();
		auto& blockChainConfig = const_cast<model::BlockChainConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).BlockChain);
		blockChainConfig.ImportanceGrouping = 1;
		blockChainConfig.TotalChainImportance = Importance(100);
		auto& nodeConfig = const_cast<config::NodeConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).Node);
		nodeConfig.OutgoingConnections = CreateConfiguration();
		auto settings = SelectorSettings(serviceState.state(), ionet::ServiceIdentifier(1));
		auto selector = CreateRemoveOnlyNodeSelector(settings);

		// Act:
		auto removeCandidates = selector();

		// Assert:
		EXPECT_TRUE(removeCandidates.empty());
	}

	// endregion

	// region CreateAgePeersTask

	namespace {
		void RunAgePeersTask(
				test::ServiceTestState& serviceState,
				net::PacketWriters& packetWriters,
				ionet::ServiceIdentifier serviceId,
				const RemoveOnlyNodeSelector& selector = RemoveOnlyNodeSelector()) {
			// Arrange:
			auto& blockChainConfig = const_cast<model::BlockChainConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).BlockChain);
			blockChainConfig.ImportanceGrouping = 1;
			blockChainConfig.TotalChainImportance = Importance(100);
			auto& nodeConfig = const_cast<config::NodeConfiguration&>(serviceState.state().pluginManager().configHolder()->Config(Height{0}).Node);
			nodeConfig.OutgoingConnections = CreateConfiguration();

			// Act:
			auto settings = SelectorSettings(serviceState.state(), serviceId);
			auto task = selector
					? CreateAgePeersTask(settings, packetWriters, selector)
					: CreateAgePeersTask(settings, packetWriters);
			auto result = task.Callback().get();

			// Assert:
			EXPECT_EQ("age peers task", task.Name);
			EXPECT_EQ(thread::TaskResult::Continue, result);
		}
	}

	TEST(TEST_CLASS, AgePeersTask_MatchingServiceNodesAreAged) {
		// Assert:
		AssertMatchingServiceNodesAreAged([](auto& serviceState, auto& writers, auto serviceId) {
			RunAgePeersTask(serviceState, writers, serviceId);
		});
	}

	TEST(TEST_CLASS, AgePeersTask_RemoveCandidatesAreClosedInWriters) {
		// Assert:
		AssertRemoveCandidatesAreClosedInWriters([](auto& serviceState, auto& writers, auto serviceId, const auto& removeCandidates) {
			RunAgePeersTask(serviceState, writers, serviceId, [&removeCandidates]() {
				return removeCandidates;
			});
		});
	}

	// endregion
}}
