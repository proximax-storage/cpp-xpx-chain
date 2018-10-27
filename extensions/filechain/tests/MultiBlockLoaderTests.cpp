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

#include "filechain/src/MultiBlockLoader.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/local/LocalNodeTestState.h"
#include "tests/test/other/mocks/MockEntityObserver.h"
#include "tests/TestHarness.h"

namespace catapult { namespace filechain {

#define TEST_CLASS MultiBlockLoaderTests

	// region CreateBlockDependentEntityObserverFactory

	namespace {
		enum class ObserverFactoryResult { Transient, Permanent, Unknown };

		ObserverFactoryResult RunObserverFactoryInflectionPointTest(
				const model::Block& lastBlock,
				Height currentBlockHeight,
				Timestamp currentBlockTime) {
			// Arrange:
			auto pCurrentBlock = test::GenerateBlockWithTransactions(0, currentBlockHeight, currentBlockTime);

			// - create configuration
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.MaxDifficultyBlocks = 100;
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(2);
			config.MaxRollbackBlocks = 22;

			// - create observers
			mocks::MockEntityObserver transientObserver;
			mocks::MockEntityObserver permanentObserver;

			// Act:
			auto observerFactory = CreateBlockDependentEntityObserverFactory(lastBlock, config, transientObserver, permanentObserver);
			const auto& observer = observerFactory(*pCurrentBlock);

			// Assert:
			return &transientObserver == &observer
					? ObserverFactoryResult::Transient
					: &permanentObserver == &observer ? ObserverFactoryResult::Permanent : ObserverFactoryResult::Unknown;
		}
	}

	TEST(TEST_CLASS, ObserverFactoryRespectsHeightInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(1234), Timestamp(utils::TimeSpan::FromHours(2).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto height) {
			return RunObserverFactoryInflectionPointTest(lastBlock, height, Timestamp());
		};

		// Act + Assert: inflection point is `LastBlockHeight - MaxDifficultyBlocks + 1` [1234 - 100 + 1]
		auto inflectionHeight = Height(1234 - 100 + 1);
		EXPECT_EQ(ObserverFactoryResult::Permanent, runTest(inflectionHeight - Height(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionHeight));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionHeight + Height(1)));
	}

	TEST(TEST_CLASS, ObserverFactoryReturnsTransientObserverWhenThereIsNoHeightInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(50), Timestamp(utils::TimeSpan::FromHours(2).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto height) {
			return RunObserverFactoryInflectionPointTest(lastBlock, height, Timestamp());
		};

		// Act + Assert: there is no height inflection point because `LastBlockHeight < MaxDifficultyBlocks` [50 < 100]
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Height(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Height(2)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Height(50)));
	}

	TEST(TEST_CLASS, ObserverFactoryRespectsTimeInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(1234), Timestamp(utils::TimeSpan::FromHours(2).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto time) {
			return RunObserverFactoryInflectionPointTest(lastBlock, Height(1), time);
		};

		// Act + Assert: inflection point is `LastBlockTime - TransactionCacheDuration` [2H - (1H + 22 * 2s)]
		auto inflectionTime = Timestamp(utils::TimeSpan::FromHours(1).millis() - 22 * utils::TimeSpan::FromSeconds(2).millis());
		EXPECT_EQ(ObserverFactoryResult::Permanent, runTest(inflectionTime - Timestamp(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionTime));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(inflectionTime + Timestamp(1)));
	}

	TEST(TEST_CLASS, ObserverFactoryReturnsTransientObserverWhenThereIsNoTimeInflectionPoint) {
		// Arrange:
		auto pLastBlock = test::GenerateBlockWithTransactions(0, Height(1234), Timestamp(utils::TimeSpan::FromHours(1).millis()));
		auto runTest = [&lastBlock = *pLastBlock](auto time) {
			return RunObserverFactoryInflectionPointTest(lastBlock, Height(1), time);
		};

		// Act + Assert: there is no time inflection point because `LastBlockTime < TransactionCacheDuration` [1H < (1H + 22 * 2s)]
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Timestamp(0)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Timestamp(1)));
		EXPECT_EQ(ObserverFactoryResult::Transient, runTest(Timestamp(utils::TimeSpan::FromHours(1).millis())));
	}

	// endregion

	// region LoadBlockChain

	namespace {
		auto MakeObserverFactory(const observers::EntityObserver& observer, std::vector<Height>& heights) {
			return [&observer, &heights](const auto& block) -> const observers::EntityObserver& {
				heights.push_back(block.Height);
				return observer;
			};
		}
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsZeroBlocksWhenStorageHeightIsOne) {
		// Arrange:
		mocks::MockEntityObserver observer;
		std::vector<Height> factoryHeights;
		test::LocalNodeTestState state;

		// Act:
		LoadBlockChain(MakeObserverFactory(observer, factoryHeights), state.ref(), Height(2));

		// Assert:
		EXPECT_EQ(0u, observer.blockHeights().size());
		EXPECT_EQ(0u, factoryHeights.size());
	}

	namespace {
		void SetStorageChainHeight(io::BlockStorageModifier&& storage, size_t height) {
			for (auto i = 2u; i <= height; ++i) {
				auto pBlock = test::GenerateBlockWithTransactions(0, Height(i), Timestamp(i * 3000));
				pBlock->CumulativeDifficulty = Difficulty(Difficulty() + i);
				storage.saveBlock(test::BlockToBlockElement(*pBlock));
			}
		}
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsSingleBlockWhenStorageHeightIsTwo) {
		// Arrange:
		mocks::MockEntityObserver observer;
		std::vector<Height> factoryHeights;
		test::LocalNodeTestState state;
		SetStorageChainHeight(state.ref().Storage.modifier(), 2);

		// Act:
		LoadBlockChain(MakeObserverFactory(observer, factoryHeights), state.ref(), Height(2));

		// Assert:
		auto expectedHeights = std::vector<Height>{ Height(2) };
		EXPECT_EQ(1u, observer.blockHeights().size());
		EXPECT_EQ(expectedHeights, observer.blockHeights());
		EXPECT_EQ(expectedHeights, factoryHeights);
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsMultipleBlocksWhenStorageHeightIsGreaterThanTwo) {
		// Arrange:
		mocks::MockEntityObserver observer;
		std::vector<Height> factoryHeights;
		test::LocalNodeTestState state;
		SetStorageChainHeight(state.ref().Storage.modifier(), 7);

		// Act:
		LoadBlockChain(MakeObserverFactory(observer, factoryHeights), state.ref(), Height(2));

		// Assert:
		auto expectedHeights = std::vector<Height>{ Height(2), Height(3), Height(4), Height(5), Height(6), Height(7) };
		EXPECT_EQ(6u, observer.blockHeights().size());
		EXPECT_EQ(expectedHeights, observer.blockHeights());
		EXPECT_EQ(expectedHeights, factoryHeights);
	}

	TEST(TEST_CLASS, LoadBlockChainLoadsMultipleBlocksStartingAtArbitraryHeight) {
		// Arrange: create a storage with 7 blocks
		mocks::MockEntityObserver observer;
		std::vector<Height> factoryHeights;
		test::LocalNodeTestState state;
		SetStorageChainHeight(state.ref().Storage.modifier(), 7);

		// Act: load blocks 4-7
		LoadBlockChain(MakeObserverFactory(observer, factoryHeights), state.ref(), Height(4));

		// Assert:
		auto expectedHeights = std::vector<Height>{ Height(4), Height(5), Height(6), Height(7) };
		EXPECT_EQ(4u, observer.blockHeights().size());
		EXPECT_EQ(expectedHeights, observer.blockHeights());
		EXPECT_EQ(expectedHeights, factoryHeights);
	}

	// endregion
}}
