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

#include "catapult/cache/CatapultCache.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/state/AccountState.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Observer test context that wraps an observer context.
	template<typename TCacheFactory>
	class ObserverTestContextT {
	public:
		/// Creates a test context around \a mode and \a height.
		explicit ObserverTestContextT(observers::NotifyMode mode, Height height)
				: m_config(config::BlockchainConfiguration::Uninitialized())
				, m_cache(TCacheFactory::Create())
				, m_cacheDelta(m_cache.createDelta())
				, m_observerState(m_cacheDelta, m_state, m_blockStatementBuilder, m_notifications)
				, m_context(m_observerState, m_config, height, Timestamp(0), mode, CreateResolverContextXor())
		{}

		/// Creates a test context around \a mode, \a height and \a config.
		explicit ObserverTestContextT(observers::NotifyMode mode, Height height, const config::BlockchainConfiguration& config)
				: m_config(config)
				, m_cache(TCacheFactory::Create(m_config))
				, m_cacheDelta(m_cache.createDelta())
				, m_observerState(m_cacheDelta, m_state, m_blockStatementBuilder, m_notifications)
				, m_context(m_observerState, m_config, height, Timestamp(0), mode, CreateResolverContextXor())
		{}

		/// Creates a test context around \a mode, \a height and \a gracePeriodDuration.
		explicit ObserverTestContextT(observers::NotifyMode mode, Height height, BlockDuration gracePeriodDuration)
				: m_config(config::BlockchainConfiguration::Uninitialized())
				, m_cache(TCacheFactory::Create(gracePeriodDuration))
				, m_cacheDelta(m_cache.createDelta())
				, m_observerState(m_cacheDelta, m_state, m_blockStatementBuilder, m_notifications)
				, m_context(m_observerState, m_config, height, Timestamp(0), mode, CreateResolverContextXor())
		{}

		/// Creates a test context around \a mode, \a height, \a config and \a gracePeriodDuration.
		explicit ObserverTestContextT(observers::NotifyMode mode, Height height, const config::BlockchainConfiguration& config, BlockDuration gracePeriodDuration)
				: m_cache(TCacheFactory::Create(config, gracePeriodDuration))
				, m_cacheDelta(m_cache.createDelta())
				, m_observerState(m_cacheDelta, m_state, m_blockStatementBuilder, m_notifications)
				, m_context(m_observerState, config, height, Timestamp(0), mode, CreateResolverContextXor())
		{}

	public:
		/// Gets the observer context.
		observers::ObserverContext& observerContext() {
			return m_context;
		}

		/// Gets the catapult cache delta.
		const cache::CatapultCacheDelta& cache() const {
			return m_cacheDelta;
		}

		/// Gets the catapult cache delta.
		cache::CatapultCacheDelta& cache() {
			return m_cacheDelta;
		}

		/// Gets the catapult state.
		const state::CatapultState& state() const {
			return m_state;
		}

		/// Gets the catapult state.
		state::CatapultState& state() {
			return m_state;
		}

		/// Gets the block statement builder.
		model::BlockStatementBuilder& statementBuilder() {
			return m_blockStatementBuilder;
		}

	public:
		/// Commits all changes to the underlying cache.
		void commitCacheChanges() {
			m_cache.commit(Height());
		}

	private:
		config::BlockchainConfiguration m_config;
		cache::CatapultCache m_cache;
		cache::CatapultCacheDelta m_cacheDelta;
		state::CatapultState m_state;
		model::BlockStatementBuilder m_blockStatementBuilder;

		std::vector<std::unique_ptr<model::Notification>> m_notifications;
		observers::ObserverState m_observerState;
		observers::ObserverContext m_context;
	};

	/// A default observer test context that wraps a cache composed of core caches only.
	class ObserverTestContext : public ObserverTestContextT<CoreSystemCacheFactory> {
		using ObserverTestContextT::ObserverTestContextT;
	};
}}
