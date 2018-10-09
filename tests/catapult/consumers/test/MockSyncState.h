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

#include "catapult/consumers/BlockChainSyncConsumer.h"
#include "tests/catapult/extensions/test/LocalNodeStateUtils.h"

namespace catapult { namespace test {

		struct MockSyncState : consumers::SyncState {
		public:
			MockSyncState() = default;

			explicit MockSyncState(cache::CatapultCache&& cache, const model::BlockElement& parentBlock)
					: consumers::SyncState(test::LocalNodeStateUtils::CreateLocalNodeStateRef(std::move(cache)))
					, m_parentBlock(parentBlock)
					{}

			explicit MockSyncState(cache::CatapultCache&& currentCache, cache::CatapultCache&& previousCache, const model::BlockElement& parentBlock)
					: consumers::SyncState(test::LocalNodeStateUtils::CreateLocalNodeStateRef(std::move(currentCache), std::move(previousCache)))
					, m_parentBlock(parentBlock)
			{}

		public:
			consumers::WeakBlockInfo commonBlockInfo() const override {
				return consumers::WeakBlockInfo(m_parentBlock);
			}

		private:
			const model::BlockElement& m_parentBlock;
		};
	}
}
