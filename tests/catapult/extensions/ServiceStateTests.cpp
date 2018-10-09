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

#include "catapult/cache/MemoryUtCache.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/thread/MultiServicePool.h"
#include "tests/catapult/extensions/test/LocalNodeStateUtils.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/other/mocks/MockNodeSubscriber.h"
#include "tests/test/other/mocks/MockStateChangeSubscriber.h"
#include "tests/test/other/mocks/MockTransactionStatusSubscriber.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS ServiceStateTests

	TEST(TEST_CLASS, CanCreateServiceState) {
		// Arrange:
		auto config = test::CreateUninitializedLocalNodeConfiguration();
		const_cast<utils::FileSize&>(config.Node.MaxPacketDataSize) = utils::FileSize::FromKilobytes(1234);

		ionet::NodeContainer nodes;
		extensions::LocalNodeStateRef state = test::LocalNodeStateUtils::CreateLocalNodeStateRef(std::move(config));
		auto pUtCache = test::CreateUtCacheProxy();

		auto numTimeSupplierCalls = 0u;
		auto timeSupplier = [&numTimeSupplierCalls]() {
			++numTimeSupplierCalls;
			return Timestamp(111);
		};

		mocks::MockTransactionStatusSubscriber transactionStatusSubscriber;
		mocks::MockStateChangeSubscriber stateChangeSubscriber;
		mocks::MockNodeSubscriber nodeSubscriber;

		std::vector<utils::DiagnosticCounter> counters;
		plugins::PluginManager pluginManager(config.BlockChain, plugins::StorageConfiguration());
		thread::MultiServicePool pool("test", 1);

		// Act:
		auto serviceState = ServiceState(
				state,
				nodes,
				*pUtCache,
				timeSupplier,
				transactionStatusSubscriber,
				stateChangeSubscriber,
				nodeSubscriber,
				counters,
				pluginManager,
				pool);

		// Assert:
		// - check references
		EXPECT_EQ(&state.Config, &serviceState.config());
		EXPECT_EQ(&nodes, &serviceState.nodes());
		EXPECT_EQ(&state.CurrentCache, &serviceState.currentCache());
		EXPECT_EQ(&state.Storage, &serviceState.storage());
		EXPECT_EQ(pUtCache.get(), &serviceState.utCache());

		EXPECT_EQ(&transactionStatusSubscriber, &serviceState.transactionStatusSubscriber());
		EXPECT_EQ(&stateChangeSubscriber, &serviceState.stateChangeSubscriber());
		EXPECT_EQ(&nodeSubscriber, &serviceState.nodeSubscriber());

		EXPECT_EQ(&counters, &serviceState.counters());
		EXPECT_EQ(&pluginManager, &serviceState.pluginManager());
		EXPECT_EQ(&pool, &serviceState.pool());

		// - check functions
		EXPECT_EQ(Timestamp(111), serviceState.timeSupplier()());
		EXPECT_EQ(1u, numTimeSupplierCalls);

		// - check empty
		EXPECT_TRUE(serviceState.tasks().empty());

		EXPECT_EQ(0u, serviceState.packetHandlers().size());
		EXPECT_EQ(1234u * 1024, serviceState.packetHandlers().maxPacketDataSize()); // should be initialized from config

		EXPECT_TRUE(serviceState.hooks().chainSyncedPredicate()); // just check that hooks is valid and default predicate can be called
		EXPECT_TRUE(serviceState.packetIoPickers().pickMatching(utils::TimeSpan::FromSeconds(1), ionet::NodeRoles::None).empty());
	}
}}
