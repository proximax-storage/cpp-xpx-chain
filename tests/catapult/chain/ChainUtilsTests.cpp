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

#include "catapult/constants.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/TimeSpan.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/TestHarness.h"

namespace catapult { namespace chain {

#define TEST_CLASS ChainUtilsTests

	// region IsChainLink

	namespace {
		model::UniqueEntityPtr<model::Block> GenerateBlockAtHeight(Height height, Timestamp timestamp) {
			auto pBlock = test::GenerateBlockWithTransactions(0, height);
			pBlock->Timestamp = timestamp;
			return pBlock;
		}

		void LinkHashes(model::Block& parentBlock, model::Block& childBlock) {
			childBlock.PreviousBlockHash = model::CalculateHash(parentBlock);
		}

		void AssertNotLinkedForHeights(Height parentHeight, Height childHeight) {
			// Arrange:
			auto pParent = GenerateBlockAtHeight(parentHeight, Timestamp(100));
			auto pChild = GenerateBlockAtHeight(childHeight, Timestamp(101));
			LinkHashes(*pParent, *pChild);

			// Act:
			bool isLink = IsChainLink(*pParent, pChild->PreviousBlockHash, *pChild);

			// Assert:
			EXPECT_FALSE(isLink) << "parent " << parentHeight << ", child " << childHeight;
		}
	}

	TEST(TEST_CLASS, IsChainLinkReturnsFalseWhenHeightIsMismatched) {
		// Assert:
		AssertNotLinkedForHeights(Height(70), Height(60));
		AssertNotLinkedForHeights(Height(70), Height(69));
		AssertNotLinkedForHeights(Height(70), Height(70));
		AssertNotLinkedForHeights(Height(70), Height(72));
		AssertNotLinkedForHeights(Height(70), Height(80));
	}

	TEST(TEST_CLASS, IsChainLinkReturnsFalseWhenPreviousBlockHashIsIncorrect) {
		// Arrange:
		auto pParent = GenerateBlockAtHeight(Height(70), Timestamp(100));
		auto pChild = GenerateBlockAtHeight(Height(71), Timestamp(101));
		LinkHashes(*pParent, *pChild);

		// Act:
		bool isLink = IsChainLink(*pParent, test::GenerateRandomByteArray<Hash256>(), *pChild);

		// Assert:
		EXPECT_FALSE(isLink);
	}

	namespace {
		void AssertNotLinkedForTimestamps(Timestamp parentTimestamp, Timestamp childTimestamp) {
			// Arrange:
			auto pParent = GenerateBlockAtHeight(Height(90), parentTimestamp);
			auto pChild = GenerateBlockAtHeight(Height(91), childTimestamp);
			LinkHashes(*pParent, *pChild);

			// Act:
			bool isLink = IsChainLink(*pParent, pChild->PreviousBlockHash, *pChild);

			// Assert:
			EXPECT_FALSE(isLink) << "parent " << parentTimestamp << ", child " << childTimestamp;
		}
	}

	TEST(TEST_CLASS, IsChainLinkReturnsFalseWhenTimestampsAreNotIncreasing) {
		// Assert:
		AssertNotLinkedForTimestamps(Timestamp(70), Timestamp(60));
		AssertNotLinkedForTimestamps(Timestamp(70), Timestamp(69));
		AssertNotLinkedForTimestamps(Timestamp(70), Timestamp(70));
	}

	namespace {
		void AssertLinkedForTimestamps(Timestamp parentTimestamp, Timestamp childTimestamp) {
			// Arrange:
			auto pParent = GenerateBlockAtHeight(Height(90), parentTimestamp);
			auto pChild = GenerateBlockAtHeight(Height(91), childTimestamp);
			LinkHashes(*pParent, *pChild);

			// Act:
			bool isLink = IsChainLink(*pParent, pChild->PreviousBlockHash, *pChild);

			// Assert:
			EXPECT_TRUE(isLink) << "parent " << parentTimestamp << ", child " << childTimestamp;
		}
	}

	TEST(TEST_CLASS, IsChainLinkReturnsTrueWhenBothHeightAndHashesAreCorrectAndTimestampsAreIncreasing) {
		// Assert:
		AssertLinkedForTimestamps(Timestamp(70), Timestamp(71));
		AssertLinkedForTimestamps(Timestamp(70), Timestamp(700));
		AssertLinkedForTimestamps(Timestamp(70), Timestamp(12345));
	}

	// endregion

	// region CheckDifficulties

	namespace {
		using DifficultySet = cache::BlockDifficultyCacheTypes::PrimaryTypes::BaseSetType::SetType::MemorySetType;

		Timestamp CalculateTimestamp(Height height, const utils::TimeSpan& timeBetweenBlocks) {
			return Timestamp((height - Height(1)).unwrap() * timeBetweenBlocks.millis());
		}

		std::unique_ptr<cache::BlockDifficultyCache> SeedBlockDifficultyCache(
				Height maxHeight,
				const utils::TimeSpan& timeBetweenBlocks,
				const model::NetworkConfiguration& config) {
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pCache = std::make_unique<cache::BlockDifficultyCache>(pConfigHolder);
			{
				auto delta = pCache->createDelta(Height{0});
				state::BlockDifficultyInfo baseInfo(Height(1));
				baseInfo.BlockDifficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
				delta->insert(baseInfo);
				pCache->commit();
			}

			for (auto height = Height(2); height <= maxHeight; height = height + Height(1)) {
				state::BlockDifficultyInfo nextBlockInfo(height, CalculateTimestamp(height, timeBetweenBlocks), Difficulty(0));
				nextBlockInfo.BlockDifficulty = CalculateDifficulty(*pCache, nextBlockInfo, config);
				auto delta = pCache->createDelta(Height{0});
				delta->insert(nextBlockInfo);
				pCache->commit();
			}

			return pCache;
		}

		std::vector<model::UniqueEntityPtr<model::Block>> GenerateBlocks(
				Height startHeight,
				const utils::TimeSpan& timeBetweenBlocks,
				uint32_t numBlocks,
				const cache::BlockDifficultyCache& cache,
				const model::NetworkConfigurations& configs) {
			std::vector<model::UniqueEntityPtr<model::Block>> blocks;
			DifficultySet difficulties;
			for (auto i = 0u; i < numBlocks; ++i) {
				auto pBlock = test::GenerateEmptyRandomBlock();
				pBlock->Height = startHeight + Height(i);
				pBlock->Timestamp = CalculateTimestamp(pBlock->Height, timeBetweenBlocks);

				auto iter = configs.lower_bound(pBlock->Height);
				if (iter->first != pBlock->Height)
					--iter;
				const auto& config = iter->second;
				if (difficulties.size() < config.MaxDifficultyBlocks) {
					auto height = difficulties.empty() ? startHeight : difficulties.begin()->BlockHeight;
					if (height > Height(1)) {
						auto range = cache.createView(height)->difficultyInfos(height - Height(1), config.MaxDifficultyBlocks - difficulties.size());
						difficulties.insert(range.begin(), range.end());
					}
				}

				auto startDifficultyIter = difficulties.cbegin();
				if (difficulties.size() > config.MaxDifficultyBlocks) {
					startDifficultyIter = difficulties.cend();
					for (auto k = 0u; k < config.MaxDifficultyBlocks; ++k)
						--startDifficultyIter;
				}

				pBlock->Difficulty = CalculateDifficulty(
						cache::DifficultyInfoRange(startDifficultyIter, difficulties.cend()),
						state::BlockDifficultyInfo(*pBlock),
						config
				);

				difficulties.insert(state::BlockDifficultyInfo(*pBlock));

				blocks.push_back(std::move(pBlock));
			}

			return blocks;
		}

		std::vector<const model::Block*> Unwrap(const std::vector<model::UniqueEntityPtr<model::Block>>& blocks) {
			std::vector<const model::Block*> blockPointers;
			for (const auto& pBlock : blocks)
				blockPointers.push_back(pBlock.get());

			return blockPointers;
		}

		model::NetworkConfiguration CreateConfiguration(uint64_t blockGenerationTargetTimeMilliseconds, uint32_t maxDifficultyBlocks, uint32_t blockTimeSmoothingFactor = 0) {
			auto config = model::NetworkConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromMilliseconds(blockGenerationTargetTimeMilliseconds);
			config.MaxDifficultyBlocks = maxDifficultyBlocks;
			config.BlockTimeSmoothingFactor = blockTimeSmoothingFactor;
			return config;
		}
	}

	TEST(TEST_CLASS, DifficultiesAreValidWhenPeerChainIsEmpty) {
		// Arrange: set up the config
		auto config = CreateConfiguration(10000, 15);

		// - seed the difficulty cache with 20 infos
		auto pCache = SeedBlockDifficultyCache(Height(20), config.BlockGenerationTargetTime, config);

		// Act: check the difficulties of an empty chain
		auto result = CheckDifficulties(*pCache, {}, config::CreateMockConfigurationHolder(config), {});

		// Assert:
		EXPECT_EQ(0u, result);
	}

	namespace {
		void AssertDifficultiesAreValidForBlocksWithEqualDifficulties(uint32_t maxDifficultyBlocks, Height chainHeight) {
			// Arrange: set up the config
			auto blockGenerationTargetTime = utils::TimeSpan::FromMilliseconds(10000);
			auto initialConfig = CreateConfiguration(blockGenerationTargetTime.millis(), maxDifficultyBlocks);
			model::NetworkConfigurations configs{{Height(1), initialConfig}};

			// - seed the difficulty cache with chainHeight infos
			auto pCache = SeedBlockDifficultyCache(chainHeight, blockGenerationTargetTime, initialConfig);

			// - generate a peer chain with five blocks
			auto blocks = GenerateBlocks(chainHeight, blockGenerationTargetTime, 5, *pCache, configs);

			// Act:
			auto result = CheckDifficulties(*pCache, Unwrap(blocks), config::CreateMockConfigurationHolder(initialConfig), configs);

			// Assert:
			EXPECT_EQ(5u, result);
		}
	}

	TEST(TEST_CLASS, DifficultiesAreValidWhenAllDifficultiesAreCorrectAndFullHistoryIsPresent_Equal) {
		// Assert:
		AssertDifficultiesAreValidForBlocksWithEqualDifficulties(4, Height(20));
	}

	TEST(TEST_CLASS, DifficultiesAreValidWhenAllDifficultiesAreCorrectAndPartialHistoryIsPresent_Equal) {
		// Assert:
		AssertDifficultiesAreValidForBlocksWithEqualDifficulties(4, Height(5));
	}

	namespace {
		void AssertDifficultiesAreValidForBlocksWithIncreasingDifficulties(uint32_t maxDifficultyBlocks, Height chainHeight) {
			// Arrange: set up the configs
			auto initialConfig = CreateConfiguration(10000, maxDifficultyBlocks, 5000);
			model::NetworkConfigurations configs{{Height(1), initialConfig}};
			auto numBlocks = 7u;
			for (auto i = 0u; i < numBlocks; i += 2)
				configs.emplace(chainHeight + Height(i),
					CreateConfiguration(10000 + i * 1000, maxDifficultyBlocks * (i + 1), 5000 + i * 1000));

			// - seed the difficulty cache with chainHeight infos and copy all the infos
			auto pCache = SeedBlockDifficultyCache(chainHeight, utils::TimeSpan::FromMilliseconds(18000), initialConfig);
			DifficultySet set;

			// - generate a peer chain with seven blocks
			auto blocks = GenerateBlocks(chainHeight, utils::TimeSpan::FromMilliseconds(8000), numBlocks, *pCache, configs);
			auto newDifficulty = Difficulty();
			for (const auto& pBlock : blocks) {
				auto iter = configs.lower_bound(pBlock->Height);
				if (iter->first != pBlock->Height)
					--iter;
				const auto& config = iter->second;
				if (set.size() < config.MaxDifficultyBlocks) {
					auto height = set.empty() ? chainHeight : set.begin()->BlockHeight;
					if (height > Height(1)) {
						auto range = pCache->createView(height)->difficultyInfos(height - Height(1), config.MaxDifficultyBlocks - set.size());
						set.insert(range.begin(), range.end());
					}
				}

				auto startDifficultyIter = set.cbegin();
				if (set.size() > config.MaxDifficultyBlocks) {
					startDifficultyIter = set.cend();
					for (auto k = 0u; k < config.MaxDifficultyBlocks; ++k)
						--startDifficultyIter;
				}

				// - calculate the expected difficulty
				newDifficulty = CalculateDifficulty(cache::DifficultyInfoRange(startDifficultyIter, set.cend()), state::BlockDifficultyInfo(*pBlock), config);
				set.insert(state::BlockDifficultyInfo(pBlock->Height, pBlock->Timestamp, pBlock->Difficulty));

				// Sanity:
				EXPECT_NE(Difficulty(), newDifficulty);
				EXPECT_LE(newDifficulty, pBlock->Difficulty);
			}

			// Act:
			auto result = CheckDifficulties(*pCache, Unwrap(blocks), config::CreateMockConfigurationHolder(initialConfig), configs);

			// Assert:
			EXPECT_EQ(7u, result);
		}
	}

	TEST(TEST_CLASS, DifficultiesAreValidWhenAllDifficultiesAreCorrectAndFullHistoryIsPresent_Increasing) {
		// Assert:
		AssertDifficultiesAreValidForBlocksWithIncreasingDifficulties(4, Height(20));
	}

	TEST(TEST_CLASS, DifficultiesAreValidWhenAllDifficultiesAreCorrectAndPartialHistoryIsPresent_Increasing) {
		// Assert:
		AssertDifficultiesAreValidForBlocksWithIncreasingDifficulties(4, Height(5));
	}

	namespace {
		void AssertDifficultiesAreInvalidForDifferenceAt(
				uint32_t maxDifficultyBlocks,
				Height chainHeight,
				uint32_t peerChainSize,
				size_t differenceIndex) {
			// Arrange: set up the configs
			auto initialConfig = CreateConfiguration(10000, maxDifficultyBlocks);
			model::NetworkConfigurations configs{{Height(1), initialConfig}};
			for (auto i = 0u; i < peerChainSize; i += 2)
				configs.emplace(chainHeight + Height(i),
					CreateConfiguration(10000 - i * 1000, maxDifficultyBlocks - i, 5000 - i * 500));

			// - seed the difficulty cache with chainHeight infos
			auto pCache = SeedBlockDifficultyCache(chainHeight, initialConfig.BlockGenerationTargetTime, initialConfig);

			// - generate a peer chain with peerChainSize blocks and change the difficulty of the block
			//   at differenceIndex
			auto blocks = GenerateBlocks(chainHeight, initialConfig.BlockGenerationTargetTime, peerChainSize, *pCache, configs);
			blocks[differenceIndex]->Difficulty = Difficulty(1);

			// Act:
			auto result = CheckDifficulties(*pCache, Unwrap(blocks), config::CreateMockConfigurationHolder(initialConfig), configs);

			// Assert:
			EXPECT_EQ(differenceIndex, result);
		}
	}

	TEST(TEST_CLASS, DifficultiesAreInvalidWhenFirstBlockHasIncorrectDifficulty) {
		// Assert:
		AssertDifficultiesAreInvalidForDifferenceAt(15, Height(20), 5, 0);
	}

	TEST(TEST_CLASS, DifficultiesAreInvalidWhenMiddleBlockHasIncorrectDifficulty) {
		// Assert:
		AssertDifficultiesAreInvalidForDifferenceAt(15, Height(20), 5, 2);
	}

	TEST(TEST_CLASS, DifficultiesAreInvalidWhenLastBlockHasIncorrectDifficulty) {
		// Assert:
		AssertDifficultiesAreInvalidForDifferenceAt(15, Height(20), 5, 4);
	}

	// endregion

	// region CalculatePartialChainScore

	namespace {
		auto CreateBlock(Timestamp timestamp, Difficulty difficulty) {
			auto pBlock = std::make_unique<model::Block>();
			pBlock->Timestamp = timestamp;
			pBlock->Difficulty = difficulty;
			return pBlock;
		}
	}

	TEST(TEST_CLASS, CanCalculatePartialChainScoreForEmptyChain) {
		// Arrange:
		auto pParentBlock = CreateBlock(Timestamp(100'000), Difficulty());

		// Act:
		auto score = CalculatePartialChainScore(*pParentBlock, {});

		// Assert:
		EXPECT_EQ(model::ChainScore(0), score);
	}

	TEST(TEST_CLASS, CanCalculatePartialChainScoreForSingleBlockChain) {
		// Arrange:
		auto pParentBlock = CreateBlock(Timestamp(100'000), Difficulty());
		std::vector<std::unique_ptr<model::Block>> blocks;
		blocks.push_back(CreateBlock(Timestamp(150'000), Difficulty() + Difficulty::Unclamped(111)));

		// Act:
		auto score = CalculatePartialChainScore(*pParentBlock, { blocks[0].get() });

		// Assert:
		EXPECT_EQ(model::ChainScore(166186883546932897), score);
	}

	TEST(TEST_CLASS, CanCalculatePartialChainScoreForMultiBlockChain) {
		// Arrange:
		auto pParentBlock = CreateBlock(Timestamp(100'000), Difficulty());
		std::vector<std::unique_ptr<model::Block>> blocks;
		blocks.push_back(CreateBlock(Timestamp(150'000), Difficulty() + Difficulty::Unclamped(111)));
		blocks.push_back(CreateBlock(Timestamp(175'000), Difficulty() + Difficulty::Unclamped(200)));
		blocks.push_back(CreateBlock(Timestamp(190'000), Difficulty() + Difficulty::Unclamped(300)));

		// Act:
		auto score = CalculatePartialChainScore(*pParentBlock, { blocks[0].get(), blocks[1].get(), blocks[2].get() });

		// Assert:
		EXPECT_EQ(model::ChainScore(319909750827845827), score);
	}

	// endregion
}}
