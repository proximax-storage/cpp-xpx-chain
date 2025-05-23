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

#include "harvesting/src/ScheduledHarvesterTask.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

using catapult::crypto::KeyPair;

namespace catapult { namespace harvesting {

#define TEST_CLASS ScheduledHarvesterTaskTests

	namespace {
		constexpr Timestamp Max_Time(std::numeric_limits<int64_t>::max());
		constexpr Timestamp Min_Time(0);
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		auto CreateConfigHolder() {
			test::MutableBlockchainConfiguration config;

			config.Network.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(60);
			config.Network.BlockTimeSmoothingFactor = 0;
			config.Network.MaxDifficultyBlocks = 60;
			config.Network.ImportanceGrouping = 123;
			config.Network.TotalChainImportance = test::Default_Total_Chain_Importance;

			config.Node.FeeInterest = 1;
			config.Node.FeeInterestDenominator = 2;

			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		struct TaskOptionsWithCounters : ScheduledHarvesterTaskOptions {
			TaskOptionsWithCounters()
					: NumHarvestingAllowedCalls(0)
					, NumLastBlockElementSupplierCalls(0)
					, NumTimeSupplierCalls(0)
					, NumRangeConsumerCalls(0)
					, BlockHeight(0)
					, BlockSigner()
					, pLastBlock(std::make_shared<model::Block>())
					, LastBlockHash(test::GenerateRandomByteArray<Hash256>()) {
				HarvestingAllowed = [this]() {
					++NumHarvestingAllowedCalls;
					return true;
				};
				LastBlockElementSupplier = [this]() {
					++NumLastBlockElementSupplierCalls;

					auto pLastElement = std::make_shared<model::BlockElement>(*pLastBlock);
					pLastElement->EntityHash = LastBlockHash;
					return pLastElement;
				};
				TimeSupplier = [this]() {
					++NumTimeSupplierCalls;
					return Max_Time;
				};
				RangeConsumer = [this](const auto& range, const auto& processingComplete) {
					++NumRangeConsumerCalls;
					const auto& block = *range.cbegin();
					BlockHeight = block.Height;
					BlockSigner = block.Signer;
					CompletionFunction = processingComplete;
				};
				pLastBlock->Size = sizeof(model::BlockHeader);
				pLastBlock->Height = Height(1);
				pLastBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
				pLastBlock->Timestamp = Timestamp(Max_Time.unwrap() / 2);
			}

			size_t NumHarvestingAllowedCalls;
			size_t NumLastBlockElementSupplierCalls;
			size_t NumTimeSupplierCalls;
			size_t NumRangeConsumerCalls;
			Height BlockHeight;
			Key BlockSigner;
			std::shared_ptr<model::Block> pLastBlock;
			Hash256 LastBlockHash;
			disruptor::ProcessingCompleteFunc CompletionFunction;
		};

		void AddDifficultyInfo(cache::CatapultCache& cache, const model::Block& block) {
			auto delta = cache.createDelta();
			auto& difficultyCache = delta.sub<cache::BlockDifficultyCache>();
			state::BlockDifficultyInfo info(block.Height, block.Timestamp, block.Difficulty);
			difficultyCache.insert(info);
			cache.commit(Height());
		}

		KeyPair AddImportantAccount(cache::CatapultCache& cache) {
			auto keyPair = KeyPair::FromPrivate(test::GenerateRandomPrivateKey());
			auto delta = cache.createDelta();
			auto& accountStateCache = delta.sub<cache::AccountStateCache>();
			accountStateCache.addAccount(keyPair.publicKey(), Height(1));
			auto& balances = accountStateCache.find(keyPair.publicKey()).get().Balances;
			balances.credit(Harvesting_Mosaic_Id, Amount(1'000'000'000'000'000), Height(1));
			balances.track(Harvesting_Mosaic_Id);
			cache.commit(Height(1));
			return test::CopyKeyPair(keyPair);
		}

		void UnlockAccount(UnlockedAccounts& unlockedAccounts, const KeyPair& keyPair) {
			auto modifier = unlockedAccounts.modifier();
			modifier.add(test::CopyKeyPair(keyPair));
		}

		struct HarvesterContext {
			HarvesterContext(const model::Block& lastBlock)
					: pConfigHolder(CreateConfigHolder())
					, Cache(test::CreateEmptyCatapultCache(pConfigHolder->Config()))
					, Accounts(1) {
				AddDifficultyInfo(Cache, lastBlock);
			}

			std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder;
			cache::CatapultCache Cache;
			UnlockedAccounts Accounts;
		};

		auto CreateHarvester(HarvesterContext& context) {
			return std::make_unique<Harvester>(context.Cache, context.pConfigHolder, Key(), context.Accounts, [](const auto& blockHeader, auto, const auto&) {
				auto pBlock = utils::MakeUniqueWithSize<model::Block>(sizeof(model::Block));
				std::memcpy(static_cast<void*>(pBlock.get()), &blockHeader, sizeof(model::BlockHeader));
				return pBlock;
			});
		}
	}

	TEST(TEST_CLASS, TaskIsShortCircuitedWhenHarvestingIsNotAllowed) {
		// Arrange:
		TaskOptionsWithCounters options;
		options.HarvestingAllowed = [&options]() {
			++options.NumHarvestingAllowedCalls;
			return false;
		};
		HarvesterContext context(*options.pLastBlock);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// Act:
		task.harvest();

		// Assert:
		EXPECT_EQ(1u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(0u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(0u, options.NumTimeSupplierCalls);
		EXPECT_EQ(0u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(0), options.BlockHeight);
		EXPECT_EQ(Key(), options.BlockSigner);
	}

	TEST(TEST_CLASS, TaskIsShortCircuitedWhenTimeIsNewBlockIsLowerThanTimeOfPreviousBlock) {
		// Arrange:
		TaskOptionsWithCounters options;
		options.TimeSupplier = [&options]() {
			++options.NumTimeSupplierCalls;
			return Min_Time;
		};
		HarvesterContext context(*options.pLastBlock);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// Act:
		task.harvest();

		// Assert:
		EXPECT_EQ(1u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(1u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(1u, options.NumTimeSupplierCalls);
		EXPECT_EQ(0u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(0), options.BlockHeight);
		EXPECT_EQ(Key(), options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsNotCalledWhenNoBlockIsHarvested) {
		// Arrange:
		TaskOptionsWithCounters options;
		HarvesterContext context(*options.pLastBlock);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// Act: no block can be harvested since no account is unlocked
		task.harvest();

		// Assert:
		EXPECT_EQ(1u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(1u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(1u, options.NumTimeSupplierCalls);
		EXPECT_EQ(0u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(0), options.BlockHeight);
		EXPECT_EQ(Key(), options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsCalledWhenBlockIsHarvested) {
		// Arrange:
		TaskOptionsWithCounters options;
		HarvesterContext context(*options.pLastBlock);
		auto keyPair = AddImportantAccount(context.Cache);
		UnlockAccount(context.Accounts, keyPair);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// Act:
		task.harvest();

		// Assert:
		EXPECT_EQ(1u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(1u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(1u, options.NumTimeSupplierCalls);
		EXPECT_EQ(1u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(2), options.BlockHeight);
		EXPECT_EQ(keyPair.publicKey(), options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsNotCalledWhenLastHarvestedBlockIsStillBeingProcessed) {
		// Arrange:
		TaskOptionsWithCounters options;
		HarvesterContext context(*options.pLastBlock);
		auto keyPair = AddImportantAccount(context.Cache);
		UnlockAccount(context.Accounts, keyPair);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// Act:
		task.harvest();
		task.harvest();

		// Assert: the second harvest did not push a block to the consumer
		// - the check for the processing complete flag is done before anything else
		EXPECT_EQ(1u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(1u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(1u, options.NumTimeSupplierCalls);
		EXPECT_EQ(1u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(2), options.BlockHeight);
		EXPECT_EQ(keyPair.publicKey(), options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsCalledAgainAfterLastHarvestedBlockWasCompletelyProcessed) {
		// Arrange:
		TaskOptionsWithCounters options;
		HarvesterContext context(*options.pLastBlock);
		auto keyPair = AddImportantAccount(context.Cache);
		UnlockAccount(context.Accounts, keyPair);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// Act:
		task.harvest();
		options.CompletionFunction(0, disruptor::ConsumerCompletionResult());
		task.harvest();

		// Assert: the second harvest did push a block to the consumer
		EXPECT_EQ(2u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(2u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(2u, options.NumTimeSupplierCalls);
		EXPECT_EQ(2u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(2), options.BlockHeight);
		EXPECT_EQ(keyPair.publicKey(), options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsCalledAgainWhenSubsequentHarvestProducesBlock) {
		// Arrange:
		TaskOptionsWithCounters options;
		HarvesterContext context(*options.pLastBlock);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// - harvest once (no account is unlocked)
		task.harvest();

		// Sanity:
		EXPECT_EQ(0u, options.NumRangeConsumerCalls);

		// Act: unlock an account and harvest again
		auto keyPair = AddImportantAccount(context.Cache);
		UnlockAccount(context.Accounts, keyPair);
		task.harvest();

		// Assert: the second harvest triggered a call to the consumer
		EXPECT_EQ(2u, options.NumHarvestingAllowedCalls);
		EXPECT_EQ(2u, options.NumLastBlockElementSupplierCalls);
		EXPECT_EQ(2u, options.NumTimeSupplierCalls);
		EXPECT_EQ(1u, options.NumRangeConsumerCalls);
		EXPECT_EQ(Height(2), options.BlockHeight);
		EXPECT_EQ(keyPair.publicKey(), options.BlockSigner);
	}
}}
