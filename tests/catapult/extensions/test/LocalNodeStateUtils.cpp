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


#include "LocalNodeStateUtils.h"

namespace catapult { namespace test {

		std::vector<std::shared_ptr<extensions::LocalNodeState>> LocalNodeStateUtils::memory;

		std::shared_ptr<extensions::LocalNodeState> LocalNodeStateUtils::CreateLocalNodeState(
				config::LocalNodeConfiguration&& config, cache::CatapultCache&& cache) {
			LocalNodeStateUtils::memory.push_back(
					std::make_shared<extensions::LocalNodeState>(
							std::move(config),
							std::make_unique<mocks::MockMemoryBasedStorage>(),
							std::move(cache)
					)
			);

			return LocalNodeStateUtils::memory.back();
		}

		std::shared_ptr<extensions::LocalNodeState> LocalNodeStateUtils::CreateLocalNodeState(config::LocalNodeConfiguration&& config) {
			LocalNodeStateUtils::memory.push_back(
					std::make_shared<extensions::LocalNodeState>(
							std::move(config),
							std::make_unique<mocks::MockMemoryBasedStorage>()
					)
			);

			return LocalNodeStateUtils::memory.back();
		}

		std::shared_ptr<extensions::LocalNodeState> LocalNodeStateUtils::CreateLocalNodeState(cache::CatapultCache&& cache) {
			auto blockConfig = model::BlockChainConfiguration::Uninitialized();
			blockConfig.EffectiveBalanceRange = 1440;

			auto LocalConfig = config::LocalNodeConfiguration(
					std::move(blockConfig),
					config::NodeConfiguration::Uninitialized(),
					config::LoggingConfiguration::Uninitialized(),
					config::UserConfiguration::Uninitialized()
			);
			return LocalNodeStateUtils::CreateLocalNodeState(std::move(LocalConfig), std::move(cache));
		}

		std::shared_ptr<extensions::LocalNodeState> LocalNodeStateUtils::CreateLocalNodeState() {
			return LocalNodeStateUtils::CreateLocalNodeState(cache::CatapultCache({}));
		}

		extensions::LocalNodeStateRef LocalNodeStateUtils::CreateLocalNodeStateRef(cache::CatapultCache&& cache) {
			return extensions::LocalNodeStateRef(*LocalNodeStateUtils::CreateLocalNodeState(std::move(cache)));
		}

		extensions::LocalNodeStateRef LocalNodeStateUtils::CreateLocalNodeStateRef(config::LocalNodeConfiguration&& config) {
			return extensions::LocalNodeStateRef(*LocalNodeStateUtils::CreateLocalNodeState(std::move(config)));
		}
	}
}