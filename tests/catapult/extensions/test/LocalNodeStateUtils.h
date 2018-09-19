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

#include <bits/unique_ptr.h>
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "tests/test/core/mocks/MockMemoryBasedStorage.h"

#pragma once

namespace catapult { namespace test {

		class LocalNodeStateUtils {
		public:

			static extensions::LocalNodeStateRef CreateLocalNodeStateRef(cache::CatapultCache&& cache);

			static extensions::LocalNodeStateRef CreateLocalNodeStateRef(config::LocalNodeConfiguration&& config);

			static std::shared_ptr<extensions::LocalNodeState> CreateLocalNodeState(config::LocalNodeConfiguration&& config);

			static std::shared_ptr<extensions::LocalNodeState> CreateLocalNodeState(cache::CatapultCache&& cache);

			static std::shared_ptr<extensions::LocalNodeState> CreateLocalNodeState(
					config::LocalNodeConfiguration&& config, cache::CatapultCache&& cache);

		private:
			static std::vector<std::shared_ptr<extensions::LocalNodeState>> memory;
		};
	}
}
