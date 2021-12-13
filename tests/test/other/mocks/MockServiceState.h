/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "MockStorageState.h"
#include "MockNodeSubscriber.h"
#include "MockStateChangeSubscriber.h"
#include "MockBlockChangeSubscriber.h"
#include "MockTransactionStatusSubscriber.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/thread/MultiServicePool.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include <utility>

namespace catapult { namespace mocks {

#pragma pack(push, 1)

    /// Mock serviceState.
    class MockServiceState {
    public:
        MockServiceState(extensions::ServiceState serviceState) : m_serviceState(std::move(serviceState)) {};

    public:
        extensions::ServiceState* ServiceState() {
            return &m_serviceState;
        }

    private:
        extensions::ServiceState m_serviceState;
    };

    std::shared_ptr<MockServiceState> CreateMockServiceState() {
        auto config = test::CreateUninitializedBlockchainConfiguration();
        const_cast<utils::FileSize&>(config.Node.MaxPacketDataSize) = utils::FileSize::FromKilobytes(1234);

        ionet::NodeContainer nodes;
        auto catapultCache = cache::CatapultCache({});
        state::CatapultState catapultState;
        io::BlockStorageCache storage(
                std::make_unique<mocks::MockMemoryBlockStorage>(),
                std::make_unique<mocks::MockMemoryBlockStorage>()
        );
        extensions::LocalNodeChainScore score;
        auto pUtCache = test::CreateUtCacheProxy();

        auto numTimeSupplierCalls = 0u;
        auto timeSupplier = [&numTimeSupplierCalls]() {
            ++numTimeSupplierCalls;
            return Timestamp(111);
        };

        mocks::MockTransactionStatusSubscriber transactionStatusSubscriber;
        mocks::MockStateChangeSubscriber stateChangeSubscriber;
        mocks::MockNodeSubscriber nodeSubscriber;
        mocks::MockBlockChangeSubscriber postBlockCommitSubscriber;

        std::vector<utils::DiagnosticCounter> counters;
        auto pConfigHolder = config::CreateMockConfigurationHolder(config);

        plugins::PluginManager pluginManager(pConfigHolder, plugins::StorageConfiguration());
        pluginManager.setStorageState(std::make_unique<mocks::MockStorageState>());

        thread::MultiServicePool pool("mock", 1);

        // Act:
        auto state = extensions::ServiceState(
                nodes,
                catapultCache,
                catapultState,
                storage,
                score,
                *pUtCache,
                timeSupplier,
                transactionStatusSubscriber,
                stateChangeSubscriber,
                nodeSubscriber,
                postBlockCommitSubscriber,
                counters,
                pluginManager,
                pool
        );

        state.hooks().setTransactionRangeConsumerFactory([](auto) { return [](auto&&) {}; });

        return std::make_shared<MockServiceState>(state);
    }

#pragma pack(pop)
}}
