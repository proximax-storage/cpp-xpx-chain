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

#include "harvesting/src/BlockStateHashCalculator.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/constants.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/TestHarness.h"

namespace catapult { namespace harvesting {

#define TEST_CLASS BlockStateHashCalculatorTests

	namespace {
		auto CreateConfiguration(bool enableVerifiableState) {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.ShouldEnableVerifiableState = enableVerifiableState;
			config.ImportanceGrouping = 123;
			config.BlockPruneInterval = 10;
			return config;
		}

		void ZeroTransactionFees(model::Block& block) {
			for (auto& transaction : block.Transactions())
				transaction.Fee = Amount(0);
		}

		template<typename TAssertHashes>
		void RunTest(bool enableVerifiableState, Height blockHeight, uint32_t numTransactions, TAssertHashes assertHashes) {
			// Arrange:
			auto pBlock = test::GenerateBlockWithTransactions(numTransactions, blockHeight, Timestamp());
			ZeroTransactionFees(*pBlock);

			test::TempDirectoryGuard dbDirGuard("testdb");
			auto config = CreateConfiguration(enableVerifiableState);
			auto cacheConfig = cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);
			auto cache = test::CreateEmptyCatapultCache(config, cacheConfig);
			{
				// add the block signer to the cache to avoid state hash changes when there are no transactions
				auto cacheDelta = cache.createDelta();
				cacheDelta.sub<cache::AccountStateCache>().addAccount(pBlock->Signer, Height(1));

				// recalculate the state hash and commit changes
				cacheDelta.calculateStateHash(Height(1));
				cache.commit(Height(1));
			}

			auto pPluginManager = test::CreatePluginManager(config);
			pPluginManager->addTransactionSupport(mocks::CreateMockTransactionPlugin());

			auto preCacheStateHash = cache.createView().calculateStateHash().StateHash;

			// Act:
			auto blockStateHashResult = CalculateBlockStateHash(*pBlock, cache, config, *pPluginManager);
			auto postCacheStateHash = cache.createView().calculateStateHash().StateHash;

			// Assert: cache state hash should not change
			EXPECT_EQ(preCacheStateHash, postCacheStateHash);

			// - check the block state hash
			assertHashes(preCacheStateHash, blockStateHashResult.first, blockStateHashResult.second);
		}

		void SetCacheHeight(cache::CatapultCache& cache, Height height) {
			auto delta = cache.createDelta();
			cache.commit(height);
		}
	}

	TEST(TEST_CLASS, ZeroHashIsReturnedWhenVerifiableStateIsDisabled) {
		// Act:
		RunTest(false, Height(2), 3, [](const auto&, const auto& blockStateHash, auto blockStateHashResult) {
			// Assert:
			EXPECT_TRUE(blockStateHashResult);

			EXPECT_EQ(Hash256(), blockStateHash);
		});
	}

	TEST(TEST_CLASS, ZeroHashIsReturnedWhenBlockIsInconsistentWithCacheState) {
		// Act: block must be *next* block, so one ahead of the cache
		for (auto height : { Height(1), Height(3), Height(10) }) {
			RunTest(true, height, 3, [height](const auto&, const auto& blockStateHash, auto blockStateHashResult) {
				// Assert:
				EXPECT_FALSE(blockStateHashResult) << height;

				EXPECT_EQ(Hash256(), blockStateHash) << height;
			});
		}
	}

	TEST(TEST_CLASS, NonZeroHashIsReturnedWhenVerifiableStateIsEnabled_NoTransactions) {
		// Act:
		RunTest(true, Height(2), 0, [](const auto& cacheStateHash, const auto& blockStateHash, auto blockStateHashResult) {
			// Assert: block does not trigger any account state changes, so hashes should be the same
			EXPECT_TRUE(blockStateHashResult);

			EXPECT_NE(Hash256(), blockStateHash);
			EXPECT_EQ(cacheStateHash, blockStateHash);
		});
	}

	TEST(TEST_CLASS, NonZeroHashIsReturnedWhenVerifiableStateIsEnabled_WithTransactions) {
		// Act:
		RunTest(true, Height(2), 3, [](const auto& cacheStateHash, const auto& blockStateHash, auto blockStateHashResult) {
			// Assert: transactions trigger account state changes, so hashes should not be the same
			EXPECT_TRUE(blockStateHashResult);

			EXPECT_NE(Hash256(), blockStateHash);
			EXPECT_NE(cacheStateHash, blockStateHash);
		});
	}

	namespace {
		void AssertDifferentBlocks(const model::Block& block1, const model::Block& block2) {
			// Arrange:
			test::TempDirectoryGuard dbDirGuard("testdb");
			auto config = CreateConfiguration(true);
			auto cacheConfig = cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);
			auto cache = test::CreateEmptyCatapultCache(config, cacheConfig);
			SetCacheHeight(cache, Height(1)); // set cache height to nemesis height

			auto pPluginManager = test::CreatePluginManager(config);
			pPluginManager->addTransactionSupport(mocks::CreateMockTransactionPlugin());

			// Act:
			auto blockStateHashResult1 = CalculateBlockStateHash(block1, cache, config, *pPluginManager);
			auto blockStateHashResult2 = CalculateBlockStateHash(block2, cache, config, *pPluginManager);

			// Assert:
			EXPECT_TRUE(blockStateHashResult1.second);
			EXPECT_TRUE(blockStateHashResult2.second);

			EXPECT_NE(blockStateHashResult1.first, blockStateHashResult2.first);
		}
	}

	TEST(TEST_CLASS, DifferentBlocksYieldDifferentStateHashes) {
		// Arrange:
		auto pBlock1 = test::GenerateBlockWithTransactions(0, Height(2), Timestamp());
		auto pBlock2 = test::CopyBlock(*pBlock1);
		test::FillWithRandomData(pBlock2->Signer);

		// Act + Assert:
		AssertDifferentBlocks(*pBlock1, *pBlock2);
	}

	TEST(TEST_CLASS, DifferentTransactionsYieldDifferentStateHashes) {
		// Arrange:
		auto pBlock1 = test::GenerateBlockWithTransactions(3, Height(2), Timestamp());
		ZeroTransactionFees(*pBlock1);

		auto pBlock2 = test::CopyBlock(*pBlock1);
		test::FillWithRandomData(pBlock2->Transactions().begin()->Signer);

		// Act + Assert:
		AssertDifferentBlocks(*pBlock1, *pBlock2);
	}

	namespace {
		DECLARE_OBSERVER(TransactionHashCapture, model::TransactionNotification)(utils::HashSet& capturedHashes) {
			return MAKE_OBSERVER(TransactionHashCapture, model::TransactionNotification, [&capturedHashes](
					const auto& notification,
					const auto&) {
				capturedHashes.insert(notification.TransactionHash);
			});
		}

		void AssertGeneratedTransactionEntityHashesAreUnique(uint32_t numTransactions) {
			// Arrange:
			test::TempDirectoryGuard dbDirGuard("testdb");
			auto config = CreateConfiguration(true);
			auto cacheConfig = cache::CacheConfiguration(dbDirGuard.name(), utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);
			auto cache = test::CreateEmptyCatapultCache(config, cacheConfig);
			SetCacheHeight(cache, Height(1));

			utils::HashSet capturedHashes;
			auto pPluginManager = test::CreatePluginManager(config);
			pPluginManager->addTransactionSupport(mocks::CreateMockTransactionPlugin());
			pPluginManager->addObserverHook([&capturedHashes](auto& builder) {
				builder.add(CreateTransactionHashCaptureObserver(capturedHashes));
			});

			auto pBlock = test::GenerateBlockWithTransactionsAtHeight(numTransactions, Height(2));
			ZeroTransactionFees(*pBlock);

			// Act: calculate state hash
			CalculateBlockStateHash(*pBlock, cache, config, *pPluginManager);

			// Assert: all transaction hashes are unique
			EXPECT_EQ(numTransactions, capturedHashes.size());
		}
	}

	TEST(TEST_CLASS, GeneratedTransactionEntityHashesAreUnique) {
		// Assert:
		AssertGeneratedTransactionEntityHashesAreUnique(3);
	}

	TEST(TEST_CLASS, GeneratedTransactionEntityHashesAreUniqueWhenMoreThanMaxByteTransactions) {
		// Assert:
		AssertGeneratedTransactionEntityHashesAreUnique(260);
	}
}}
