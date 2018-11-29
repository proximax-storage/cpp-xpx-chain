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
#include "harvesting/src/Harvester.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/TestHarness.h"

using catapult::crypto::KeyPair;

namespace catapult { namespace harvesting {

#define TEST_CLASS ScheduledHarvesterTaskTests

	namespace {
		constexpr Timestamp Max_Time(std::numeric_limits<int64_t>::max());

		model::BlockChainConfiguration CreateConfiguration() {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(60);
			config.BlockTimeSmoothingFactor = 0;
			config.MaxDifficultyBlocks = 60;
			config.ImportanceGrouping = 123;
			return config;
		}

		struct TaskOptionsWithCounters : ScheduledHarvesterTaskOptions {
			TaskOptionsWithCounters()
					: NumHarvestingAllowedCalls(0)
					, NumLastBlockElementSupplierCalls(0)
					, NumTimeSupplierCalls(0)
					, NumRangeConsumerCalls(0)
					, BlockHeight(0)
					, BlockSigner{}
					, pLastBlock(std::make_shared<model::Block>())
					, LastBlockHash(test::GenerateRandomData<Hash256_Size>()) {
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
				pLastBlock->Size = sizeof(model::Block);
				pLastBlock->Height = Height(1);
				pLastBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
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
			accountStateCache.find(keyPair.publicKey()).get().Balances.credit(Xpx_Id, Amount(1'000'000'000'000'000), Height(1));
			cache.commit(Height());
			return test::CopyKeyPair(keyPair);
		}

		void UnlockAccount(UnlockedAccounts& unlockedAccounts, const KeyPair& keyPair) {
			auto modifier = unlockedAccounts.modifier();
			modifier.add(test::CopyKeyPair(keyPair));
		}

		struct HarvesterContext {
			HarvesterContext(const model::Block& lastBlock)
					: Config(CreateConfiguration())
					, Cache(test::CreateEmptyCatapultCache(Config))
					, Accounts(1) {
				AddDifficultyInfo(Cache, lastBlock);
			}

			model::BlockChainConfiguration Config;
			cache::CatapultCache Cache;
			UnlockedAccounts Accounts;
		};

		auto CreateHarvester(HarvesterContext& context) {
			Harvester::Suppliers harvesterSuppliers{
				[](const auto&) { return std::make_pair(Hash256(), true); },
				[](auto) { return TransactionsInfo(); }
			};
			return std::make_unique<Harvester>(context.Cache, context.Config, context.Accounts, harvesterSuppliers);
		}
	}

	TEST(TEST_CLASS, TaskIsShortCircuitedIfHarvestingIsNotAllowed) {
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
		EXPECT_EQ(Key{}, options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsNotCalledIfNoBlockWasHarvested) {
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
		EXPECT_EQ(Key{}, options.BlockSigner);
	}

	TEST(TEST_CLASS, BlockConsumerIsCalledIfBlockWasHarvested) {
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

	TEST(TEST_CLASS, BlockConsumerIsNotCalledMultipleTimesIfLastHarvestedBlockWasNotCompletelyProcessedYet) {
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

	TEST(TEST_CLASS, BlockConsumerIsCalledMultipleTimesWhenLastHarvestedBlockWasCompletelyProcessed) {
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

	TEST(TEST_CLASS, BlockConsumerIsCalledIfLaterHarvestProducesBlock) {
		// Arrange:
		TaskOptionsWithCounters options;
		HarvesterContext context(*options.pLastBlock);
		ScheduledHarvesterTask task(options, CreateHarvester(context));

		// - harvest once (no account is unlocked)
		task.harvest();

		// Sanity:
		EXPECT_EQ(0u, options.NumRangeConsumerCalls);

		// Act: unlock an accuont and harvest again
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
