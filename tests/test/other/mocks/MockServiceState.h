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
        MockServiceState()
        : m_serviceState(
                m_nodes,
                m_cache,
                m_state,
                m_storage,
                m_score,
                *m_pUtCache,
                m_timeSupplier,
                m_transactionStatusSubscriber,
                m_stateChangeSubscriber,
                m_nodeSubscriber,
                m_postBlockCommitSubscriber,
                m_counters,
                m_pluginManager,
                m_pool
          )
        {
            m_pluginManager.setStorageState(std::make_unique<mocks::MockStorageState>());
            m_serviceState.hooks().setTransactionRangeConsumerFactory([](auto) { return [](auto&&) {}; });
        };

    public:
        extensions::ServiceState* ServiceState() {
            return &m_serviceState;
        }

    private:
        ionet::NodeContainer m_nodes = ionet::NodeContainer{};
        cache::CatapultCache m_cache = cache::CatapultCache({});
        state::CatapultState m_state = state::CatapultState{};
        io::BlockStorageCache m_storage = io::BlockStorageCache(
                std::make_unique<mocks::MockMemoryBlockStorage>(),
                std::make_unique<mocks::MockMemoryBlockStorage>()
        );
        extensions::LocalNodeChainScore m_score = extensions::LocalNodeChainScore{};
        std::unique_ptr<cache::MemoryUtCacheProxy> m_pUtCache = test::CreateUtCacheProxy();
        supplier<Timestamp> m_timeSupplier = []() {return Timestamp(111);};
        mocks::MockTransactionStatusSubscriber m_transactionStatusSubscriber = mocks::MockTransactionStatusSubscriber{};
        mocks::MockStateChangeSubscriber m_stateChangeSubscriber = mocks::MockStateChangeSubscriber{};
        mocks::MockNodeSubscriber m_nodeSubscriber = mocks::MockNodeSubscriber{};
        mocks::MockBlockChangeSubscriber m_postBlockCommitSubscriber = mocks::MockBlockChangeSubscriber{};
        std::vector<utils::DiagnosticCounter> m_counters = std::vector<utils::DiagnosticCounter>{};
        plugins::PluginManager m_pluginManager = plugins::PluginManager(
                config::CreateMockConfigurationHolder(test::CreateUninitializedBlockchainConfiguration()),
                plugins::StorageConfiguration()
        );
        thread::MultiServicePool m_pool = thread::MultiServicePool("mock", 1);

        extensions::ServiceState m_serviceState;
    };

#pragma pack(pop)
}}
