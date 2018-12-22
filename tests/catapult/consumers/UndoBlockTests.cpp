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

#include "catapult/consumers/UndoBlock.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/mocks/MockEntityObserver.h"
#include "tests/TestHarness.h"

namespace catapult { namespace consumers {

#define TEST_CLASS UndoBlockTests

	namespace {
		std::vector<uint16_t> GetExpectedVersions(uint16_t numTransactions, uint16_t seed) {
			std::vector<uint16_t> versions;
			versions.push_back(seed); // block should be processed after all transactions, so it should be undone first

			for (uint16_t i = 0u; i < numTransactions; ++i)
				versions.push_back(seed + numTransactions - i);

			return versions;
		}

		void SetVersions(model::Block& block, uint16_t seed) {
			block.Version = seed;

			for (auto& tx : block.Transactions())
				tx.Version = ++seed;
		}

		void AssertContexts(
				const std::vector<observers::ObserverContext>& contexts,
				const observers::ObserverState& state,
				Height height,
				observers::NotifyMode mode) {
			for (const auto& context : contexts) {
				EXPECT_EQ(&state.Cache, &context.Cache);
				EXPECT_EQ(&state.State, &context.State);
				EXPECT_EQ(height, context.Height);
				EXPECT_EQ(mode, context.Mode);
			}
		}
	}

	// region state hash disabled

	namespace {
		template<typename TAction>
		void RunStateHashDisabledTest(TAction action) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache();
			auto delta = cache.createDelta();
			mocks::MockEntityObserver observer;
			auto pBlock = test::GenerateBlockWithTransactionsAtHeight(7, Height(10));
			SetVersions(*pBlock, 22);

			state::CatapultState catapultState;
			observers::ObserverState state(delta, catapultState);

			auto blockElement = test::BlockToBlockElement(*pBlock);

			// Act:
			action(blockElement, observer, state);
		}
	}

	TEST(TEST_CLASS, RollbackBlockIsUndoneWhenStateHashIsDisabled) {
		// Arrange:
		RunStateHashDisabledTest([](const auto& blockElement, const auto& observer, const auto& state) {
			// Act:
			UndoBlock(blockElement, observer, state, UndoBlockType::Rollback);

			// Assert:
			EXPECT_EQ(8u, observer.versions().size());
			EXPECT_EQ(GetExpectedVersions(7, 22), observer.versions());

			EXPECT_EQ(8u, observer.contexts().size());
			AssertContexts(observer.contexts(), state, Height(10), observers::NotifyMode::Rollback);
		});
	}

	TEST(TEST_CLASS, CommonBlockIsIgnoredWhenStateHashIsDisabled) {
		// Arrange:
		RunStateHashDisabledTest([](const auto& blockElement, const auto& observer, const auto& state) {
			// Act:
			UndoBlock(blockElement, observer, state, UndoBlockType::Common);

			// Assert:
			EXPECT_TRUE(observer.versions().empty());
			EXPECT_TRUE(observer.contexts().empty());
		});
	}

	// endregion

	// region state hash enabled

	namespace {
		template<typename TAction>
		void RunStateHashEnabledTest(TAction action) {
			// Arrange:
			test::TempDirectoryGuard dbDirGuard("testdb");
			auto config = model::BlockChainConfiguration::Uninitialized();
			auto cacheConfig = cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);
			auto cache = test::CreateEmptyCatapultCache(config, cacheConfig);

			auto delta = cache.createDelta();
			mocks::MockEntityObserver observer;
			auto pBlock = test::GenerateBlockWithTransactionsAtHeight(7, Height(10));
			SetVersions(*pBlock, 22);

			state::CatapultState catapultState;
			observers::ObserverState state(delta, catapultState);

			auto blockElement = test::BlockToBlockElement(*pBlock);
			blockElement.SubCacheMerkleRoots = { Hash256() }; // trigger clear of account state cache

			// - add an account so the cache is not empty
			{
				delta.sub<cache::AccountStateCache>().addAccount(test::GenerateRandomData<Key_Size>(), Height(4));
				delta.calculateStateHash(Height(1)); // force a recalculation of the state root
				cache.commit(Height(1));
			}

			// Sanity:
			EXPECT_NE(Hash256(), delta.calculateStateHash(blockElement.Block.Height).SubCacheMerkleRoots[0]);

			// Act:
			action(blockElement, observer, state);
		}
	}

	TEST(TEST_CLASS, RollbackBlockIsUndoneWhenStateHashIsEnabled) {
		// Arrange:
		RunStateHashEnabledTest([](const auto& blockElement, const auto& observer, const auto& state) {
			// Act:
			UndoBlock(blockElement, observer, state, UndoBlockType::Rollback);

			// Assert:
			EXPECT_EQ(8u, observer.versions().size());
			EXPECT_EQ(GetExpectedVersions(7, 22), observer.versions());

			EXPECT_EQ(8u, observer.contexts().size());
			AssertContexts(observer.contexts(), state, Height(10), observers::NotifyMode::Rollback);

			// - the tree was not changed
			EXPECT_NE(Hash256(), state.Cache.calculateStateHash(blockElement.Block.Height).SubCacheMerkleRoots[0]);
		});
	}

	TEST(TEST_CLASS, CommonBlockUpdatesStateHashWhenStateHashIsEnabled) {
		// Arrange:
		RunStateHashEnabledTest([](const auto& blockElement, const auto& observer, const auto& state) {
			// Act:
			UndoBlock(blockElement, observer, state, UndoBlockType::Common);

			// Assert:
			EXPECT_TRUE(observer.versions().empty());
			EXPECT_TRUE(observer.contexts().empty());

			// - the tree was changed
			EXPECT_EQ(Hash256(), state.Cache.calculateStateHash(blockElement.Block.Height).SubCacheMerkleRoots[0]);
		});
	}

	// endregion
}}
