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

#include "catapult/chain/BlockExecutor.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/other/mocks/MockEntityObserver.h"

namespace catapult { namespace chain {

#define TEST_CLASS BlockExecutorTests

	namespace {
		constexpr auto CreateResolverContext = test::CreateResolverContextWithCustomDoublingMosaicResolver;

		void FixHashes(model::BlockElement& blockElement) {
			auto start = blockElement.Block.Signature[0];
			blockElement.EntityHash = { { start } };

			for (auto& transactionElement : blockElement.Transactions)
				transactionElement.EntityHash = { { ++start, static_cast<uint8_t>(transactionElement.Transaction.Type) } };
		}

		struct ExecuteTraits {
			static constexpr auto Notify_Mode = observers::NotifyMode::Commit;

			static std::vector<Hash256> GetExpectedHashes(const model::Block& block) {
				std::vector<Hash256> hashes;

				auto start = block.Signature[0];
				for (const auto& transaction : block.Transactions())
					hashes.push_back({ { ++start, static_cast<uint8_t>(transaction.Type) } });

				hashes.push_back({ { block.Signature[0] } });
				return hashes;
			}

			static void ProcessBlock(
					const model::Block& block,
					const observers::EntityObserver& observer,
					observers::ObserverState& state) {
				auto blockElement = test::BlockToBlockElement(block);
				FixHashes(blockElement);
				ExecuteBlock(blockElement, { observer, CreateResolverContext(), config::CreateMockConfigurationHolder(), state });
			}
		};

		struct RollbackTraits {
			static constexpr auto Notify_Mode = observers::NotifyMode::Rollback;

			static std::vector<Hash256> GetExpectedHashes(const model::Block& block) {
				auto hashes = ExecuteTraits::GetExpectedHashes(block);
				std::reverse(hashes.begin(), hashes.end());
				return hashes;
			}

			static void ProcessBlock(
					const model::Block& block,
					const observers::EntityObserver& observer,
					observers::ObserverState& state) {
				auto blockElement = test::BlockToBlockElement(block);
				FixHashes(blockElement);
				RollbackBlock(blockElement, { observer, CreateResolverContext(), config::CreateMockConfigurationHolder(), state });
			}
		};

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
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Execute) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExecuteTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Rollback) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RollbackTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(CanDispatchSingleBlockWithoutTransactions) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache();
		auto delta = cache.createDelta();
		mocks::MockEntityObserver observer;
		auto pBlock = test::GenerateBlockWithTransactions(0, Height(10));

		state::CatapultState catapultState;
		std::vector<std::unique_ptr<model::Notification>> notifications;
		observers::ObserverState state(delta, catapultState, notifications);

		// Act:
		TTraits::ProcessBlock(*pBlock, observer, state);

		// Assert:
		EXPECT_EQ(1u, observer.versions().size());

		EXPECT_EQ(1u, observer.contexts().size());
		AssertContexts(observer.contexts(), state, Height(10), TTraits::Notify_Mode);
	}

	TRAITS_BASED_TEST(CanDispatchSingleBlockWithTransactions) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache();
		auto delta = cache.createDelta();
		mocks::MockEntityObserver observer;
		auto pBlock = test::GenerateBlockWithTransactions(7, Height(10));

		state::CatapultState catapultState;
		std::vector<std::unique_ptr<model::Notification>> notifications;
		observers::ObserverState state(delta, catapultState, notifications);

		// Act:
		TTraits::ProcessBlock(*pBlock, observer, state);

		// Assert:
		EXPECT_EQ(8u, observer.versions().size());

		EXPECT_EQ(8u, observer.contexts().size());
		AssertContexts(observer.contexts(), state, Height(10), TTraits::Notify_Mode);
	}

	TRAITS_BASED_TEST(ProcessPassesAllEntityHashesToObserverWithoutModification) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache();
		auto delta = cache.createDelta();
		mocks::MockEntityObserver observer;
		auto pBlock = test::GenerateBlockWithTransactions(7, Height(10));

		state::CatapultState catapultState;
		std::vector<std::unique_ptr<model::Notification>> notifications;
		observers::ObserverState state(delta, catapultState, notifications);

		// Act:
		TTraits::ProcessBlock(*pBlock, observer, state);

		// Assert:
		const auto& hashes = observer.entityHashes();
		EXPECT_EQ(8u, hashes.size());
		EXPECT_EQ(TTraits::GetExpectedHashes(*pBlock), hashes);
	}

	TRAITS_BASED_TEST(CanDispatchMultipleBlocksWithTransactions) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache();
		auto delta = cache.createDelta();
		mocks::MockEntityObserver observer;
		auto pBlock1 = test::GenerateBlockWithTransactions(5, Height(10));
		auto pBlock2 = test::GenerateBlockWithTransactions(3, Height(25));

		state::CatapultState catapultState;
		std::vector<std::unique_ptr<model::Notification>> notifications;
		observers::ObserverState state(delta, catapultState, notifications);

		// Act:
		TTraits::ProcessBlock(*pBlock1, observer, state);
		TTraits::ProcessBlock(*pBlock2, observer, state);

		// Assert:
		const auto& versions = observer.versions();
		EXPECT_EQ(10u, versions.size());

		const auto& contexts = observer.contexts();
		EXPECT_EQ(10u, observer.contexts().size());
		auto contextsSplitIter = contexts.cbegin() + 6;
		auto mode = TTraits::Notify_Mode;
		AssertContexts(std::vector<observers::ObserverContext>(contexts.cbegin(), contextsSplitIter), state, Height(10), mode);
		AssertContexts(std::vector<observers::ObserverContext>(contextsSplitIter, contexts.cend()), state, Height(25), mode);
	}

	TEST(TEST_CLASS, RollbackCommitsAccountRemovals) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache();
		mocks::MockEntityObserver observer;
		auto pBlock = test::GenerateBlockWithTransactions(1, Height(1));

		{
			auto delta = cache.createDelta();
			state::CatapultState catapultState;
			std::vector<std::unique_ptr<model::Notification>> notifications;
			observers::ObserverState state(delta, catapultState, notifications);

			// - add three accounts and queue a removal
			Address address{ { 2 } };
			auto& accountStateCache = delta.sub<cache::AccountStateCache>();
			accountStateCache.addAccount(Address{ { 1 } }, Height(1));
			accountStateCache.addAccount(address, Height(2));
			accountStateCache.addAccount(Address{ { 3 } }, Height(3));
			accountStateCache.queueRemove(address, Height(2));

			// Sanity: three accounts should be in the cache
			EXPECT_EQ(3u, accountStateCache.size());
			EXPECT_TRUE(accountStateCache.contains(address));

			// Act: trigger a rollback
			RollbackBlock(model::BlockElement(*pBlock), { observer, CreateResolverContext(), config::CreateMockConfigurationHolder(), state });

			// Assert: the account queued for removal should have been removed
			EXPECT_EQ(2u, accountStateCache.size());
			EXPECT_FALSE(accountStateCache.contains(address));
		}

		// Sanity: the original cache was not modified at all
		EXPECT_EQ(0u, cache.sub<cache::AccountStateCache>().createView(Height{0})->size());
	}
}}
