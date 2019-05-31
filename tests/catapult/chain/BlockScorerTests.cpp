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

#include "catapult/chain/BlockScorer.h"
#include "catapult/model/Block.h"
#include "catapult/utils/Logging.h"
#include "tests/TestHarness.h"
#include "tests/test/nodeps/TestConstants.h"

namespace catapult { namespace chain {

#define TEST_CLASS BlockScorerTests

	namespace {
		model::BlockChainConfiguration CreateConfiguration() {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(60);
			config.BlockTimeSmoothingFactor = 0;
			config.TotalChainImportance = test::Default_Total_Chain_Importance;
			config.AverageBlockRecordingCost = 0.0;
			config.GreedDelta = 0.5;
			config.GreedExponent = 2.00;
			return config;
		}

		void SetTimestampSeconds(model::Block& block, uint64_t time) {
			block.Timestamp = Timestamp(time * 1'000);
		}
	}

	// region CalculateHit

	TEST(TEST_CLASS, CanCalculateHit) {
		// Arrange:
		const Hash256 generationHash{ {
			0xF7, 0xF6, 0xF5, 0xF4, 0xF3, 0xF2, 0xF1, 0xF0,
			0xE7, 0xE6, 0xE5, 0xE4, 0xE3, 0xE2, 0xE1, 0xE0,
			0xD7, 0xD6, 0xD5, 0xD4, 0xD3, 0xD2, 0xD1, 0xD0,
			0xC7, 0xC6, 0xC5, 0xC4, 0xC3, 0xC2, 0xC1, 0xC0
		} };

		// Act:
		auto hit = CalculateHit(generationHash);

		// Assert:
		EXPECT_EQ(17361925168090707703ull, hit);
	}

	TEST(TEST_CLASS, CanCalculateHitWhenGenerationHashIsZero) {
		// Arrange:
		const Hash256 generationHash{ {} };

		// Act:
		auto hit = CalculateHit(generationHash);

		// Assert:
		EXPECT_EQ(0, hit);
	}

	TEST(TEST_CLASS, CanCalculateHitWhenGenerationHashIsMax) {
		// Arrange:
		const Hash256 generationHash{ {
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		} };

		// Act:
		auto hit = CalculateHit(generationHash);

		// Assert:
		EXPECT_EQ(std::numeric_limits<uint64_t>::max(), hit);
	}

	// endregion

	// region CalculateScore

	TEST(TEST_CLASS, CanCalculateScore) {
		// Arrange:
		model::Block parent;
		SetTimestampSeconds(parent, 5567320ull);

		model::Block current;
		SetTimestampSeconds(current, 5568532ull);
		current.Difficulty = Difficulty(1 << 16);

		// Act:
		auto score = CalculateScore(parent, current);

		// Assert:
		EXPECT_EQ(1ull << (64 - 16), score);
	}

	namespace {
		uint64_t CalculateBlockScore(uint64_t parentTime, uint64_t currentTime) {
			// Arrange:
			model::Block parent;
			SetTimestampSeconds(parent, parentTime);

			model::Block current;
			SetTimestampSeconds(current, currentTime);
			current.Difficulty = Difficulty(44'888);

			// Act:
			return CalculateScore(parent, current);
		}
	}

	TEST(TEST_CLASS, BlockScoreIsZeroWhenElapsedTimeIsZero) {
		// Act:
		auto score = CalculateBlockScore(1000, 1000);

		// Assert:
		EXPECT_EQ(0u, score);
	}

	TEST(TEST_CLASS, BlockScoreIsZeroWhenElapsedTimeIsNegative) {
		// Act:
		auto score = CalculateBlockScore(1000, 900);

		// Assert:
		EXPECT_EQ(0u, score);
	}

	// endregion

	// region CalculateTarget

	namespace {
		BlockTarget CalculateTargetFromRawValues(
				uint64_t parentTime,
				uint64_t currentTime,
				uint64_t importance,
				uint64_t difficulty = 60,
				const model::BlockChainConfiguration& config = CreateConfiguration(),
				Amount maxFee = Amount{0},
				Amount fee = Amount{0}) {
			// Arrange:
			auto timeDiff = utils::TimeSpan::FromSeconds(currentTime - parentTime);
			auto realDifficulty = Difficulty(difficulty);

			// Act:
			return CalculateTarget(timeDiff, realDifficulty, Importance(importance), config, maxFee, fee);
		}

		BlockTarget CalculateBlockTarget(
				uint64_t parentTime,
				uint64_t currentTime,
				uint64_t importance,
				uint64_t difficulty = 60,
				const model::BlockChainConfiguration& config = CreateConfiguration(),
				Amount maxFee = Amount{0},
				Amount fee = Amount{0}) {
			// Arrange:
			model::Block parent;
			SetTimestampSeconds(parent, parentTime);

			model::Block current;
			SetTimestampSeconds(current, currentTime);
			current.Difficulty = Difficulty(difficulty);

			// Act:
			return CalculateTarget(parent, current, Importance(importance), config, maxFee, fee);
		}
	}

	TEST(TEST_CLASS, BlockTargetIsZeroWhenEffectiveImportanceIsZero) {
		// Act:
		auto target = CalculateBlockTarget(1000, 1100, 0);

		// Assert:
		EXPECT_EQ(BlockTarget(0), target);
	}

	TEST(TEST_CLASS, BlockTargetIsZeroWhenElapsedTimeIsZero) {
		// Act:
		auto target = CalculateBlockTarget(1000, 1000, 100);

		// Assert:
		EXPECT_EQ(BlockTarget(0), target);
	}

	TEST(TEST_CLASS, BlockTargetIsZeroWhenElapsedTimeIsNegative) {
		// Act:
		auto target = CalculateBlockTarget(1000, 900, 100);

		// Assert:
		EXPECT_EQ(BlockTarget(0), target);
	}

	TEST(TEST_CLASS, BlockTargetIncreasesAsTimeElapses) {
		// Act:
		auto target1 = CalculateBlockTarget(900, 1000, 72000);
		auto target2 = CalculateBlockTarget(900, 1100, 72000);

		// Assert:
		EXPECT_GT(target2, target1);
	}

	TEST(TEST_CLASS, BlockTargetIncreasesAsImportanceIncreases) {
		// Act:
		auto target1 = CalculateBlockTarget(900, 1000, 72000);
		auto target2 = CalculateBlockTarget(900, 1000, 74000);

		// Assert:
		EXPECT_GT(target2, target1);
	}

	TEST(TEST_CLASS, BlockTargetIncreasesAsDifficultyIncrease) {
		// Act:
		auto target1 = CalculateBlockTarget(900, 1000, 72000, 40);
		auto target2 = CalculateBlockTarget(900, 1000, 72000, 50);

		// Assert:
		EXPECT_GT(target2, target1);
	}

	TEST(TEST_CLASS, BlockTargetIsCorrectlyCalculated) {
		// Act:
		auto target = CalculateBlockTarget(900, 1000, 72000, 50);

		// Assert:
		BlockTarget expectedTarget = 100; // time difference (in seconds)
		expectedTarget *= 72000; // importance
		expectedTarget *= 50; // base target

		// Assert:
		EXPECT_EQ(expectedTarget, target);
	}

	TEST(TEST_CLASS, TargetIsCorrectlyCalculatedFromRawValues) {
		// Act:
		auto target = CalculateTargetFromRawValues(900, 1000, 72000, 50);

		// Assert:
		BlockTarget expectedTarget = 100; // time difference (in seconds)
		expectedTarget *= 72000; // importance
		expectedTarget *= 50; // difficulty

		// Assert:
		EXPECT_EQ(expectedTarget, target);
	}

	TEST(TEST_CLASS, BlockTargetIsConsistentWithLegacyBlockTarget) {
		// Act:
		auto target = CalculateBlockTarget(1, 1000001, 72000 * 8'000'000'000L);

		// Assert:
		BlockTarget expectedTarget = 72000 * 8'000'000'000L;
		expectedTarget *= 1000000;
		expectedTarget *= 60;
		EXPECT_EQ(expectedTarget, target);
	}

	TEST(TEST_CLASS, GenerationTimeHasNoImpactOnTargetWhenSmoothingIsDisabled) {
		// Act:
		auto config = CreateConfiguration();
		config.BlockTimeSmoothingFactor = 0;
		config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15);
		auto target1 = CalculateBlockTarget(900, 916, 72000, 50, config);

		config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(10);
		auto target2 = CalculateBlockTarget(900, 916, 72000, 50, config);

		// Assert:
		EXPECT_EQ(target2, target1);
	}

	// endregion

	// region BlockHitPredicate

	namespace {
		struct BlockHitPredicateContext {
			explicit BlockHitPredicateContext(Importance importance)
					: Config(CreateConfiguration())
					, Predicate(
							Config,
							[this, importance](const auto& key, auto height) {
								ImportanceLookupParams.push_back(std::make_pair(key, height));
								return importance;
							})
			{}

			model::BlockChainConfiguration Config;
			BlockHitPredicate Predicate;
			std::vector<std::pair<Key, Height>> ImportanceLookupParams;
		};

		std::unique_ptr<model::Block> CreateBlock(Height height, uint32_t timestampSeconds, uint32_t difficulty) {
			auto pBlock = std::make_unique<model::Block>();
			pBlock->Signer = test::GenerateRandomData<Hash256_Size>();
			pBlock->Height = height;
			pBlock->Difficulty = Difficulty(difficulty);
			SetTimestampSeconds(*pBlock, timestampSeconds);
			return pBlock;
		}
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsTrueWhenHitIsLessThanTarget) {
		// Arrange:
		auto pParent = CreateBlock(Height(10), 900, 0);
		auto pCurrent = CreateBlock(Height(11), 1000, 50);
		const Hash256 generationHash{ { 0xF7, 0xF6, 0xF5, 0xF4 } };
		Amount maxFee{0};
		Amount fee{0};

		Importance signerImportance = Importance(20000000);
		BlockHitPredicateContext context(signerImportance);

		// Act:
		auto isHit = context.Predicate(*pParent, *pCurrent, generationHash, maxFee, fee);

		// Assert:
		EXPECT_LT(CalculateHit(generationHash), CalculateTarget(*pParent, *pCurrent, signerImportance, context.Config, maxFee, fee));
		EXPECT_TRUE(isHit);

		ASSERT_EQ(1u, context.ImportanceLookupParams.size());
		EXPECT_EQ(pCurrent->Signer, context.ImportanceLookupParams[0].first);
		EXPECT_EQ(pCurrent->Height, context.ImportanceLookupParams[0].second);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsTrueWhenHitIsLessThanTarget_Context) {
		// Arrange:
		BlockHitContext hitContext;
		hitContext.GenerationHash = { { 0xF7, 0xF6, 0xF5, 0xF4 } };
		hitContext.ElapsedTime = utils::TimeSpan::FromSeconds(100);
		hitContext.Signer = test::GenerateRandomData<Hash256_Size>();
		hitContext.Difficulty = Difficulty(50 * 1'000'000'000'000);
		hitContext.Height = Height(11);
		hitContext.MaxFee = Amount{0};
		hitContext.Fee = Amount{0};

		Importance signerImportance = Importance(20000000);
		BlockHitPredicateContext context(signerImportance);

		// Act:
		auto isHit = context.Predicate(hitContext);

		// Assert:
		auto hit = CalculateHit(hitContext.GenerationHash);
		auto target = CalculateTarget(hitContext.ElapsedTime, hitContext.Difficulty, signerImportance, context.Config,
			hitContext.MaxFee, hitContext.Fee);
		EXPECT_LT(hit, target);
		EXPECT_TRUE(isHit);

		ASSERT_EQ(1u, context.ImportanceLookupParams.size());
		EXPECT_EQ(hitContext.Signer, context.ImportanceLookupParams[0].first);
		EXPECT_EQ(hitContext.Height, context.ImportanceLookupParams[0].second);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsEqualToTarget) {
		// Arrange:
		auto pParent = CreateBlock(Height(10), 1, 0);
		auto pCurrent = CreateBlock(Height(11), 65537 * 640 + 1, 257 * 17 * 5 * 3);
		const Hash256 generationHash{ {
			0x00, 0x40, 0x66, 0x00, 0x7F, 0xC2, 0x99, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		} };
		Amount maxFee{0};
		Amount fee{0};

		Importance signerImportance = Importance(6700417);
		BlockHitPredicateContext context(signerImportance);

		// Act:
		auto isHit = context.Predicate(*pParent, *pCurrent, generationHash, maxFee, fee);

		// Assert:
		EXPECT_EQ(CalculateHit(generationHash), CalculateTarget(*pParent, *pCurrent, signerImportance, context.Config, maxFee, fee));
		EXPECT_FALSE(isHit);

		ASSERT_EQ(1u, context.ImportanceLookupParams.size());
		EXPECT_EQ(pCurrent->Signer, context.ImportanceLookupParams[0].first);
		EXPECT_EQ(pCurrent->Height, context.ImportanceLookupParams[0].second);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsEqualToTarget_Context) {
		// Arrange:
		BlockHitContext hitContext;
		hitContext.GenerationHash = { {
			0x00, 0x40, 0x66, 0x00, 0x7F, 0xC2, 0x99, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		} };
		hitContext.ElapsedTime = utils::TimeSpan::FromSeconds(65537 * 640);
		hitContext.Signer = test::GenerateRandomData<Hash256_Size>();
		hitContext.Difficulty = Difficulty(257 * 17 * 5 * 3);
		hitContext.Height = Height(11);
		hitContext.MaxFee = Amount{0};
		hitContext.Fee = Amount{0};

		Importance signerImportance = Importance(6700417);
		BlockHitPredicateContext context(signerImportance);

		// Act:
		auto isHit = context.Predicate(hitContext);

		// Assert:
		auto hit = CalculateHit(hitContext.GenerationHash);
		auto target = CalculateTarget(hitContext.ElapsedTime, hitContext.Difficulty, signerImportance, context.Config,
			hitContext.MaxFee, hitContext.Fee);
		EXPECT_EQ(hit, target);
		EXPECT_FALSE(isHit);

		ASSERT_EQ(1u, context.ImportanceLookupParams.size());
		EXPECT_EQ(hitContext.Signer, context.ImportanceLookupParams[0].first);
		EXPECT_EQ(hitContext.Height, context.ImportanceLookupParams[0].second);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsGreaterThanTarget) {
		// Arrange:
		auto pParent = CreateBlock(Height(10), 900, 0);
		auto pCurrent = CreateBlock(Height(11), 1000, 50);
		const Hash256 generationHash{ { 0xF7, 0xF6, 0xF5, 0xF4 } };
		Amount maxFee{0};
		Amount fee{0};

		Importance signerImportance = Importance(1000);
		BlockHitPredicateContext context(signerImportance);

		// Act:
		auto isHit = context.Predicate(*pParent, *pCurrent, generationHash, maxFee, fee);

		// Assert:
		EXPECT_GT(CalculateHit(generationHash), CalculateTarget(*pParent, *pCurrent, signerImportance, context.Config, maxFee, fee));
		EXPECT_FALSE(isHit);

		ASSERT_EQ(1u, context.ImportanceLookupParams.size());
		EXPECT_EQ(pCurrent->Signer, context.ImportanceLookupParams[0].first);
		EXPECT_EQ(pCurrent->Height, context.ImportanceLookupParams[0].second);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsGreaterThanTarget_Context) {
		// Arrange:
		BlockHitContext hitContext;
		hitContext.GenerationHash = { { 0xF7, 0xF6, 0xF5, 0xF4 } };
		hitContext.ElapsedTime = utils::TimeSpan::FromSeconds(100);
		hitContext.Signer = test::GenerateRandomData<Hash256_Size>();
		hitContext.Difficulty = Difficulty(50);
		hitContext.Height = Height(11);
		hitContext.MaxFee = Amount{0};
		hitContext.Fee = Amount{0};

		Importance signerImportance = Importance(1000);
		BlockHitPredicateContext context(signerImportance);

		// Act:
		auto isHit = context.Predicate(hitContext);

		// Assert:
		auto hit = CalculateHit(hitContext.GenerationHash);
		auto target = CalculateTarget(hitContext.ElapsedTime, hitContext.Difficulty, signerImportance, context.Config,
			hitContext.MaxFee, hitContext.Fee);
		EXPECT_GT(hit, target);
		EXPECT_FALSE(isHit);

		ASSERT_EQ(1u, context.ImportanceLookupParams.size());
		EXPECT_EQ(hitContext.Signer, context.ImportanceLookupParams[0].first);
		EXPECT_EQ(hitContext.Height, context.ImportanceLookupParams[0].second);
	}

	// endregion
}}
