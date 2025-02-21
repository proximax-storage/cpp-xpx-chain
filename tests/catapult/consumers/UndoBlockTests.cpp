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
#include "catapult/chain/BlockExecutor.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/other/mocks/MockEntityObserver.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace consumers {

#define TEST_CLASS UndoBlockTests

	namespace {
		constexpr auto CreateResolverContext = test::CreateResolverContextWithCustomDoublingMosaicResolver;

		void AssertContexts(
				const std::vector<observers::ObserverContext>& contexts,
				observers::ObserverState& state,
				Height height,
				observers::NotifyMode mode) {
			// Assert:
			for (const auto& context : contexts) {
				EXPECT_EQ(&state.Cache, &context.Cache);
				EXPECT_EQ(&state.State, &context.State);
				EXPECT_EQ(height, context.Height);
				EXPECT_EQ(mode, context.Mode);

				// - appropriate resolvers were passed down
				EXPECT_EQ(MosaicId(22), context.Resolvers.resolve(UnresolvedMosaicId(11)));
			}
		}

		auto CreateConfig(bool enableUndoBlock = true) {
			test::MutableBlockchainConfiguration config;
			config.Network.EnableUndoBlock = enableUndoBlock;
			return config.ToConst();
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
			auto pBlock = test::GenerateBlockWithTransactions(7, Height(10));

			state::CatapultState catapultState;
			std::vector<std::unique_ptr<model::Notification>> notifications;
			observers::ObserverState state(delta, catapultState, notifications);

			auto blockElement = test::BlockToBlockElement(*pBlock);

			// Act:
			action(blockElement, observer, state);
		}
	}

	TEST(TEST_CLASS, RollbackBlockIsUndoneWhenStateHashIsDisabled) {
		// Arrange:
		RunStateHashDisabledTest([](const auto& blockElement, const auto& observer, auto& state) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder(CreateConfig());

			// Act:
			UndoBlock(pConfigHolder->Config().Network, blockElement, { observer, CreateResolverContext(), pConfigHolder, state }, UndoBlockType::Rollback);

			// Assert:
			EXPECT_EQ(8u, observer.versions().size());

			EXPECT_EQ(8u, observer.contexts().size());
			AssertContexts(observer.contexts(), state, Height(10), observers::NotifyMode::Rollback);
		});
	}

	TEST(TEST_CLASS, CommonBlockIsIgnoredWhenStateHashIsDisabled) {
		// Arrange:
		RunStateHashDisabledTest([](const auto& blockElement, const auto& observer, auto& state) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder(CreateConfig());

			// Act:
			UndoBlock(pConfigHolder->Config().Network, blockElement, { observer, CreateResolverContext(), pConfigHolder, state }, UndoBlockType::Common);

			// Assert:
			EXPECT_TRUE(observer.versions().empty());
			EXPECT_TRUE(observer.contexts().empty());
		});
	}

	TEST(TEST_CLASS, RollbackBlockAssertWhenUndoBlockDisabled) {
		// Arrange:
		RunStateHashDisabledTest([](const auto& blockElement, const auto& observer, auto& state) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder(CreateConfig(false));

			// Act:
			EXPECT_THROW(UndoBlock(pConfigHolder->Config().Network, blockElement, { observer, CreateResolverContext(), pConfigHolder, state }, UndoBlockType::Rollback), catapult_runtime_error);
		});
	}

	// endregion

	// region state hash enabled

	namespace {
		template<typename TAction>
		void RunStateHashEnabledTest(TAction action) {
			// Arrange:
			test::TempDirectoryGuard dbDirGuard;
			auto cacheConfig = cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);
			auto cache = test::CreateEmptyCatapultCache(cacheConfig);

			auto delta = cache.createDelta();
			mocks::MockEntityObserver observer;
			auto pBlock = test::GenerateBlockWithTransactions(7, Height(10));

			state::CatapultState catapultState;
			std::vector<std::unique_ptr<model::Notification>> notifications;
			observers::ObserverState state(delta, catapultState, notifications);

			auto blockElement = test::BlockToBlockElement(*pBlock);
			blockElement.SubCacheMerkleRoots = { Hash256(), Hash256() }; // trigger clear of account state cache

			// - add an account so the cache is not empty
			{
				delta.sub<cache::AccountStateCache>().addAccount(test::GenerateRandomByteArray<Key>(), Height(4));
				delta.calculateStateHash(Height(1)); // force a recalculation of the state root
				cache.commit(Height(1));
			}

			// Sanity:
			EXPECT_NE(Hash256(), delta.calculateStateHash(blockElement.Block.Height).SubCacheMerkleRoots[1]);

			// Act:
			action(blockElement, observer, state);
		}
	}

	TEST(TEST_CLASS, RollbackBlockIsUndoneWhenStateHashIsEnabled) {
		// Arrange:
		RunStateHashEnabledTest([](const auto& blockElement, const auto& observer, auto& state) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder(CreateConfig());

			// Act:
			UndoBlock(pConfigHolder->Config().Network, blockElement, { observer, CreateResolverContext(), pConfigHolder, state }, UndoBlockType::Rollback);

			// Assert:
			EXPECT_EQ(8u, observer.versions().size());

			EXPECT_EQ(8u, observer.contexts().size());
			AssertContexts(observer.contexts(), state, Height(10), observers::NotifyMode::Rollback);

			// - the tree was not changed
			EXPECT_NE(Hash256(), state.Cache.calculateStateHash(blockElement.Block.Height).SubCacheMerkleRoots[1]);
		});
	}

	TEST(TEST_CLASS, CommonBlockUpdatesStateHashWhenStateHashIsEnabled) {
		// Arrange:
		RunStateHashEnabledTest([](const auto& blockElement, const auto& observer, auto& state) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder(CreateConfig());

			// Act:
			UndoBlock(pConfigHolder->Config().Network, blockElement, { observer, CreateResolverContext(), pConfigHolder, state }, UndoBlockType::Common);

			// Assert:
			EXPECT_TRUE(observer.versions().empty());
			EXPECT_TRUE(observer.contexts().empty());

			// - the tree was changed
			EXPECT_EQ(Hash256(), state.Cache.calculateStateHash(blockElement.Block.Height).SubCacheMerkleRoots[0]);
		});
	}

	TEST(TEST_CLASS, CommonBlockAssertWhenUndoBlockDisabled) {
		// Arrange:
		RunStateHashEnabledTest([](const auto& blockElement, const auto& observer, auto& state) {
			// Arrange:
			auto pConfigHolder = config::CreateMockConfigurationHolder(CreateConfig(false));

			// Act:
			EXPECT_THROW(UndoBlock(pConfigHolder->Config().Network, blockElement, { observer, CreateResolverContext(), pConfigHolder, state }, UndoBlockType::Common), catapult_runtime_error);
		});
	}

	// endregion
}}
