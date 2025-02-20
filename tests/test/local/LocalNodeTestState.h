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
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/state/CatapultState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "tests/test/core/mocks/MockMemoryBlockStorage.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include <memory>
#include <string>

namespace catapult { namespace model { struct NetworkConfiguration; } }

namespace catapult { namespace test {

	/// Local node test state.
	class LocalNodeTestState {
	private:
		struct Impl {
		public:
			explicit Impl(config::BlockchainConfiguration&& config, cache::CatapultCache&& cache);

			template<typename TNemesisDataType>
			explicit Impl(config::BlockchainConfiguration&& config, cache::CatapultCache&& cache, const TNemesisDataType& data)
				: m_config(std::move(config))
				, m_cache(std::move(cache))
				, m_storage(std::make_unique<mocks::MockMemoryBlockStorage>([=](){return mocks::CreateNemesisBlockElement(data);}), std::make_unique<mocks::MockMemoryBlockStorage>([=](){return mocks::CreateNemesisBlockElement(data);}))
			{}

		public:
			extensions::LocalNodeStateRef ref();

		private:
			config::BlockchainConfiguration m_config;
			state::CatapultState m_state;
			cache::CatapultCache m_cache;
			io::BlockStorageCache m_storage;
			extensions::LocalNodeChainScore m_score;
		};
	public:
		/// Creates default state around \a config.
		explicit LocalNodeTestState(const model::NetworkConfiguration& config);

		/// Creates default state around \a config and \a userDataDirectory.
		explicit LocalNodeTestState(const model::NetworkConfiguration& config, const std::string& userDataDirectory);

		/// Creates default state around \a cache.
		explicit LocalNodeTestState(cache::CatapultCache&& cache);

		/// Creates default state around \a config, \a userDataDirectory and \a cache.
		LocalNodeTestState(
			const model::NetworkConfiguration& config,
			const std::string& userDataDirectory,
			cache::CatapultCache&& cache);

		/// Creates default state around \a config.
		explicit LocalNodeTestState(const config::BlockchainConfiguration& config);

		/// Creates default state around \a config and \a cache.
		explicit LocalNodeTestState(const config::BlockchainConfiguration& config, cache::CatapultCache&& cache);

		template<typename TNemesisDataType>
		explicit LocalNodeTestState(const config::BlockchainConfiguration& config, cache::CatapultCache&& cache, const TNemesisDataType& data)
			: m_pImpl(std::make_unique<Impl>(config::BlockchainConfiguration(config), std::move(cache), data))
		{}

		/// Destroys the state.
		~LocalNodeTestState();

	public:
		/// Returns a state ref.
		extensions::LocalNodeStateRef ref();

	private:
		std::unique_ptr<Impl> m_pImpl;
	};
}}
