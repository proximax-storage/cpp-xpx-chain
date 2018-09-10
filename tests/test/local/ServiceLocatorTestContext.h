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
#include "LocalTestUtils.h"
#include "catapult/cache/MemoryUtCache.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/thread/MultiServicePool.h"
#include "catapult/utils/NetworkTime.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/SchedulerTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBasedStorage.h"
#include "tests/test/other/mocks/MockNodeSubscriber.h"
#include "tests/test/other/mocks/MockStateChangeSubscriber.h"
#include "tests/test/other/mocks/MockTransactionStatusSubscriber.h"

namespace catapult { namespace test {

	/// Wrapper around ServiceState.
	class ServiceTestState {
	public:
		/// Creates the test state.
		ServiceTestState() : ServiceTestState(cache::CatapultCache({}))
		{}

		/// Creates the test state around \a cache.
		explicit ServiceTestState(cache::CatapultCache&& cache) : ServiceTestState(std::move(cache), &utils::NetworkTime)
		{}

		/// Creates the test state around \a cache and \a timeSupplier.
		explicit ServiceTestState(cache::CatapultCache&& cache, const supplier<Timestamp>& timeSupplier)
				: m_state(
						LoadLocalNodeConfiguration(""),
						std::make_unique<mocks::MockMemoryBasedStorage>(),
						std::move(cache)
				)
				, m_pUtCache(CreateUtCacheProxy())
				, m_pluginManager(m_state.Config.BlockChain, plugins::StorageConfiguration())
				, m_pool("service locator test context", 2)
				, m_serviceState(
						  m_state,
						  m_nodes,
						  *m_pUtCache,
						  timeSupplier,
						  m_transactionStatusSubscriber,
						  m_stateChangeSubscriber,
						  m_nodeSubscriber,
						  m_counters,
						  m_pluginManager,
						  m_pool
				)
		{}

	public:
		/// Gets the service state.
		auto& state() {
			return m_serviceState;
		}

	public:
		/// Gets the config.
		auto& config() const {
			return m_state.Config;
		}

		/// Gets the transaction status subscriber.
		const auto& transactionStatusSubscriber() const {
			return m_transactionStatusSubscriber;
		}

		/// Gets the state change subscriber.
		const auto& stateChangeSubscriber() const {
			return m_stateChangeSubscriber;
		}

		/// Gets the node subscriber.
		const auto& nodeSubscriber() const {
			return m_nodeSubscriber;
		}

		/// Gets the counters.
		auto& counters() {
			return m_counters;
		}

		/// Gets the plugin manager.
		auto& pluginManager() {
			return m_pluginManager;
		}

	private:
		extensions::LocalNodeState m_state;
		ionet::NodeContainer m_nodes;
		std::unique_ptr<cache::MemoryUtCacheProxy> m_pUtCache;

		mocks::MockTransactionStatusSubscriber m_transactionStatusSubscriber;
		mocks::MockStateChangeSubscriber m_stateChangeSubscriber;
		mocks::MockNodeSubscriber m_nodeSubscriber;

		std::vector<utils::DiagnosticCounter> m_counters;
		plugins::PluginManager m_pluginManager;
		thread::MultiServicePool m_pool;

		extensions::ServiceState m_serviceState;
	};

	/// A test context for extension service tests.
	template<typename TTraits>
	class ServiceLocatorTestContext {
	public:
		/// Creates the test context.
		ServiceLocatorTestContext()
				: m_keyPair(GenerateKeyPair())
				, m_locator(m_keyPair)
		{}

		/// Creates the test context around \a cache.
		explicit ServiceLocatorTestContext(cache::CatapultCache&& cache)
				: ServiceLocatorTestContext(std::move(cache), &utils::NetworkTime)
		{}

		/// Creates the test context around \a cache and \a timeSupplier.
		explicit ServiceLocatorTestContext(cache::CatapultCache&& cache, const supplier<Timestamp>& timeSupplier)
				: m_keyPair(GenerateKeyPair())
				, m_locator(m_keyPair)
				, m_testState(std::move(cache), timeSupplier)
		{}

	public:
		/// Gets the value of the counter named \a counterName.
		uint64_t counter(const std::string& counterName) const {
			for (const auto& counter : m_locator.counters()) {
				if (HasName(counter, counterName))
					return counter.value();
			}

			CATAPULT_THROW_INVALID_ARGUMENT_1("could not find counter with name", counterName);
		}

	public:
		/// Gets the public key.
		const auto& publicKey() const {
			return m_keyPair.publicKey();
		}

		/// Gets the service locator.
		auto& locator() {
			return m_locator;
		}

		/// Gets the service locator.
		const auto& locator() const {
			return m_locator;
		}

		/// Gets the test state.
		auto& testState() {
			return m_testState;
		}

		/// Gets the test state.
		const auto& testState() const {
			return m_testState;
		}

	public:
		/// Boots the service around \a args.
		template<typename... TArgs>
		void boot(TArgs&&... args) {
			auto pRegistrar = TTraits::CreateRegistrar(std::forward<TArgs>(args)...);
			pRegistrar->registerServiceCounters(m_locator);
			pRegistrar->registerServices(m_locator, m_testState.state());
		}

		/// Shuts down the service.
		void shutdown() {
			m_testState.state().pool().shutdown();
		}

	private:
		static bool HasName(const utils::DiagnosticCounter& counter, const std::string& name) {
			return name == counter.id().name();
		}

	private:
		crypto::KeyPair m_keyPair;
		extensions::ServiceLocator m_locator;

		ServiceTestState m_testState;
	};

	/// Extracts a task named \a taskName from \a context, which is expected to contain \a numExpectedTasks tasks,
	/// and forwards it to \a action.
	/// \note Context is expected to be booted.
	template<typename TTestContext, typename TAction>
	void RunTaskTestPostBoot(TTestContext& context, size_t numExpectedTasks, const std::string& taskName, TAction action) {
		// Sanity: expected number of tasks should be registered
		const auto& tasks = context.testState().state().tasks();
		EXPECT_EQ(numExpectedTasks, tasks.size());

		// Act: find the desired task
		auto iter = std::find_if(tasks.cbegin(), tasks.cend(), [&taskName](const auto& task) {
			return taskName == task.Name;
		});

		if (tasks.cend() == iter)
			CATAPULT_THROW_RUNTIME_ERROR_1("unable to find task with name", taskName);

		action(*iter);
	}

	/// Extracts a task named \a taskName from \a context, which is expected to contain \a numExpectedTasks tasks,
	/// and forwards it to \a action.
	template<typename TTestContext, typename TAction>
	void RunTaskTest(TTestContext& context, size_t numExpectedTasks, const std::string& taskName, TAction action) {
		// Arrange:
		context.boot();

		// Act:
		RunTaskTestPostBoot(context, numExpectedTasks, taskName, std::move(action));
	}

	/// Asserts a task named \a taskName is registered by \a context, which is expected to contain \a numExpectedTasks tasks.
	template<typename TTestContext>
	void AssertRegisteredTask(TTestContext&& context, size_t numExpectedTasks, const std::string& taskName) {
		// Act:
		test::RunTaskTest(context, numExpectedTasks, taskName, [&taskName](const auto& task) {
			// Assert:
			AssertUnscheduledTask(task, taskName);
		});
	}
}}
