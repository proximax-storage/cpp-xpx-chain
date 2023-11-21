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

#include "LocalNodeTestState.h"
#include "LocalTestUtils.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	namespace {
		auto CreateEmptyCatapultCache(const model::NetworkConfiguration& networkConfig) {
			test::MutableBlockchainConfiguration config;
			config.Network = networkConfig;
			return test::CreateEmptyCatapultCache(config.ToConst());
		}
	}

	LocalNodeTestState::Impl::Impl(config::BlockchainConfiguration&& config, cache::CatapultCache&& cache)
				: m_config(std::move(config))
				, m_cache(std::move(cache))
				, m_storage(std::make_unique<mocks::MockMemoryBlockStorage>(), std::make_unique<mocks::MockMemoryBlockStorage>())
			{}
	extensions::LocalNodeStateRef  LocalNodeTestState::Impl::ref() {
		return extensions::LocalNodeStateRef(config::CreateMockConfigurationHolder(m_config), m_state, m_cache, m_storage, m_score);
	}
	LocalNodeTestState::LocalNodeTestState(const model::NetworkConfiguration& config)
			: LocalNodeTestState(config, "", CreateEmptyCatapultCache(config))
	{}

	LocalNodeTestState::LocalNodeTestState(const model::NetworkConfiguration& config, const std::string& userDataDirectory)
			: LocalNodeTestState(config, userDataDirectory, CreateEmptyCatapultCache(config))
	{}

	LocalNodeTestState::LocalNodeTestState(cache::CatapultCache&& cache)
			: m_pImpl(std::make_unique<Impl>(CreatePrototypicalBlockchainConfiguration(), std::move(cache)))
	{}

	LocalNodeTestState::LocalNodeTestState(
			const model::NetworkConfiguration& config,
			const std::string& userDataDirectory,
			cache::CatapultCache&& cache)
			: m_pImpl(std::make_unique<Impl>(
					CreatePrototypicalBlockchainConfiguration(model::NetworkConfiguration(config), userDataDirectory),
					std::move(cache)))
	{}

	LocalNodeTestState::LocalNodeTestState(const config::BlockchainConfiguration& config)
			: m_pImpl(std::make_unique<Impl>(config::BlockchainConfiguration(config), CreateEmptyCatapultCache(config)))
	{}

	LocalNodeTestState::LocalNodeTestState(const config::BlockchainConfiguration& config, cache::CatapultCache&& cache)
			: m_pImpl(std::make_unique<Impl>(config::BlockchainConfiguration(config), std::move(cache)))
	{}

	LocalNodeTestState::~LocalNodeTestState() = default;

	extensions::LocalNodeStateRef LocalNodeTestState::ref() {
		return m_pImpl->ref();
	}
}}
