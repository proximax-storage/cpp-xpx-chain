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

#include "harvesting/src/Harvester.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/TransactionPlugin.h"
#include "harvesting/src/Harvester.h"
#include "tests/catapult/extensions/test/LocalNodeStateUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/test/local/LocalNodeTestState.h"
#include "tests/test/nodeps/Waits.h"
#include "tests/TestHarness.h"

using catapult::crypto::KeyPair;

namespace catapult { namespace harvesting {

#define TEST_CLASS HarvesterTests

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		constexpr Timestamp Max_Time(std::numeric_limits<int64_t>::max());
		constexpr size_t Num_Accounts = 5;
		constexpr Amount DefaultBalance{1000000000000000};

		std::vector<KeyPair> CreateKeyPairs(size_t count) {
			std::vector<KeyPair> keyPairs;
			for (auto i = 0u; i < count; ++i)
				keyPairs.push_back(KeyPair::FromPrivate(test::GenerateRandomPrivateKey()));

			return keyPairs;
		}

		std::vector<state::AccountState*> CreateAccounts(
				cache::AccountStateCacheDelta& cache,
				const std::vector<KeyPair>& keyPairs,
				const std::vector<Amount>& balances) {
			std::vector<state::AccountState*> accountStates;
			for (auto i = 0u; i < keyPairs.size(); ++i) {
				auto& accountState = cache.addAccount(keyPairs[i].publicKey(), Height(1));
				if (i < balances.size()) {
					accountState.Balances.credit(Xpx_Id, balances[i]);
				}
				accountStates.push_back(&accountState);
			}

			return accountStates;
		}

		void UnlockAllAccounts(UnlockedAccounts& unlockedAccounts, const std::vector<KeyPair>& keyPairs) {
			auto modifier = unlockedAccounts.modifier();
			for (const auto& keyPair : keyPairs)
				modifier.add(test::CopyKeyPair(keyPair));
		}

		std::unique_ptr<model::Block> CreateBlock() {
			auto pBlock = test::GenerateEmptyRandomBlock();
			pBlock->Height = Height(1);
			pBlock->CumulativeDifficulty = Difficulty{0};
			return pBlock;
		}

		model::BlockChainConfiguration CreateConfiguration() {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.Network.Identifier = Network_Identifier;
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15);
			config.BlockTimeSmoothingFactor = 0;
			config.MaxDifficultyBlocks = 60;
			return config;
		}

		class TestHarvester : public Harvester {
		public:
			explicit TestHarvester(
					extensions::LocalNodeStateRef localNodeState,
					const UnlockedAccounts& unlockedAccounts,
					const TransactionsInfoSupplier& transactionsInfoSupplier)
				: Harvester(localNodeState, unlockedAccounts, transactionsInfoSupplier) {}

		public:
			cache::CatapultCache& GetCurrentCache() { return m_localNodeState.CurrentCache; }
			cache::CatapultCache& GetPreviousCache() { return m_localNodeState.PreviousCache; }
		};

		struct HarvesterContext {
		public:
			HarvesterContext(
				const std::vector<Amount>& currentBalances = {Num_Accounts, DefaultBalance},
				const std::vector<Amount>& previousBalances = {Num_Accounts, DefaultBalance},
				size_t numAccounts = Num_Accounts)
					: CurrentCache(test::CreateEmptyCatapultCache(CreateConfiguration()))
					, PreviousCache(test::CreateEmptyCatapultCache(CreateConfiguration()))
					, KeyPairs(CreateKeyPairs(numAccounts))
					, pUnlockedAccounts(std::make_unique<UnlockedAccounts>(numAccounts))
					, pLastBlock(CreateBlock())
					, LastBlockElement(test::BlockToBlockElement(*pLastBlock)) {
				auto delta = CurrentCache.createDelta();
				CurrentAccountStates = CreateAccounts(delta.sub<cache::AccountStateCache>(), KeyPairs, currentBalances);
				CurrentCache.commit(Height());

				delta = PreviousCache.createDelta();
				PreviousAccountStates = CreateAccounts(delta.sub<cache::AccountStateCache>(), KeyPairs, previousBalances);
				PreviousCache.commit(Height());


				UnlockAllAccounts(*pUnlockedAccounts, KeyPairs);
				LastBlockElement.GenerationHash = test::GenerateRandomData<Hash256_Size>();
			}

		public:
			auto CreateHarvester(model::BlockChainConfiguration&& config, const TransactionsInfoSupplier& transactionsInfoSupplier) {
				pLocalNodeState = std::make_unique<test::LocalNodeTestState>(config, "", std::move(CurrentCache), std::move(PreviousCache));
				auto localNodeState = pLocalNodeState->ref();
				return std::make_unique<TestHarvester>(std::move(localNodeState), *pUnlockedAccounts, transactionsInfoSupplier);
			}

			auto CreateHarvester(model::BlockChainConfiguration&& config) {
				return CreateHarvester(std::move(config), [](size_t) { return TransactionsInfo(); });
			}

			auto CreateHarvester() {
				return CreateHarvester(CreateConfiguration());
			}

		public:
			cache::CatapultCache CurrentCache;
			cache::CatapultCache PreviousCache;
			std::vector<KeyPair> KeyPairs;
			std::vector<state::AccountState*> CurrentAccountStates;
			std::vector<state::AccountState*> PreviousAccountStates;
			std::unique_ptr<UnlockedAccounts> pUnlockedAccounts;
			std::shared_ptr<model::Block> pLastBlock;
			model::BlockElement LastBlockElement;
			std::unique_ptr<test::LocalNodeTestState> pLocalNodeState;
		};

		Key BestHarvesterKey(const model::BlockElement& lastBlockElement, const std::vector<KeyPair>& keyPairs) {
			const KeyPair* pBestKeyPair = nullptr;
			uint64_t bestHit = std::numeric_limits<uint64_t>::max();
			for (const auto& keyPair : keyPairs) {
				auto generationHash = model::CalculateGenerationHash(lastBlockElement.GenerationHash, keyPair.publicKey());
				uint64_t hit = chain::CalculateHit(generationHash);
				if (hit < bestHit) {
					bestHit = hit;
					pBestKeyPair = &keyPair;
				}
			}

			return pBestKeyPair->publicKey();
		}

		Timestamp CalculateBlockGenerationTime(const HarvesterContext& context, const Key& publicKey) {
			uint64_t hit = chain::CalculateHit(model::CalculateGenerationHash(context.LastBlockElement.GenerationHash, publicKey));
			uint64_t seconds = (BlockTarget{hit} / BlockTarget{DefaultBalance.unwrap()} / context.pLastBlock->BaseTarget).convert_to<uint64_t>();
			return Timestamp((seconds + 1) * 1000);
		}

		Amount CalculateHitBalance(const HarvesterContext& context, const Key& publicKey, const Timestamp& elapsedTime) {
			uint64_t hit = chain::CalculateHit(model::CalculateGenerationHash(context.LastBlockElement.GenerationHash, publicKey));
			auto currentBaseTarget = chain::CalculateBaseTarget(context.pLastBlock->BaseTarget,
				utils::TimeSpan::FromMilliseconds(elapsedTime.unwrap()), CreateConfiguration());
			Amount balance = Amount{(BlockTarget{hit} / BlockTarget{elapsedTime.unwrap() / 1000} / currentBaseTarget).convert_to<uint64_t>()};
			return balance;
		}
	}

	TEST(TEST_CLASS, HarvestReturnsNullptrIfCurrentBalanceZero) {
		// Arrange:
		HarvesterContext context{{}, {Num_Accounts, DefaultBalance}};
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvestReturnsNullptrIfPreviousBalanceZero) {
		// Arrange:
		HarvesterContext context{{Num_Accounts, DefaultBalance}, {}};
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvestReturnsNullptrIfPreviousAndCurrentBalancesZero) {
		// Arrange:
		HarvesterContext context{{}, {}};
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvestReturnsNullptrIfNoAccountIsUnlocked) {
		// Arrange:
		HarvesterContext context;
		{
			auto modifier = context.pUnlockedAccounts->modifier();
			for (const auto& keyPair : context.KeyPairs)
				modifier.remove(keyPair.publicKey());
		}

		auto pHarvester = context.CreateHarvester();

		// Sanity:
		EXPECT_EQ(0u, context.pUnlockedAccounts->view().size());

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvestReturnsBlockIfEnoughTimeElapsed) {
		// Arrange:
		HarvesterContext context;
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_TRUE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvesterWithBestKeyCreatesBlockAtEarliestMoment) {
		// Arrange:
		test::RunNonDeterministicTest("harvester with best key harvests", []() {
			HarvesterContext context{{DefaultBalance}, {DefaultBalance}, 1};
			auto timestamp = Timestamp{1000000};
			auto tooEarly = Timestamp{1000};
			auto hitBalance = CalculateHitBalance(context, context.KeyPairs[0].publicKey(), timestamp);
			CATAPULT_LOG(info) << "hitBalance = " << hitBalance;

			HarvesterContext context2{{hitBalance}, {hitBalance}, 1};
			context2.LastBlockElement.GenerationHash = context.LastBlockElement.GenerationHash;
			auto pHarvester = context2.CreateHarvester();

			// Sanity: harvester cannot harvest a block before it's her turn
			auto pBlock1 = pHarvester->harvest(context2.LastBlockElement, tooEarly);

			// Act: harvester should succeed at earliest possible time
			auto pBlock2 = pHarvester->harvest(context2.LastBlockElement, timestamp);
			if (!pBlock2)
				return false;

			// Assert:
			EXPECT_FALSE(!!pBlock1);
			EXPECT_TRUE(!!pBlock2);
			return true;
		});
	}

	TEST(TEST_CLASS, HarvestReturnsNullptrIfNoHarvesterHasHit) {
		// Arrange:
		HarvesterContext context;
		auto bestKey = BestHarvesterKey(context.LastBlockElement, context.KeyPairs);
		auto timestamp = CalculateBlockGenerationTime(context, bestKey);
		auto tooEarly = Timestamp(timestamp.unwrap() - 1000);
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, tooEarly);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvestReturnsNullptrIfAccountsAreUnlockedButNotFoundInCache) {
		// Arrange:
		HarvesterContext context;
		auto pHarvester = context.CreateHarvester();

		{
			auto delta = pHarvester->GetCurrentCache().createDelta();
			auto& accountStateCache = delta.sub<cache::AccountStateCache>();
			for (auto pState : context.CurrentAccountStates)
				accountStateCache.queueRemove(pState->Address, pState->AddressHeight);

			accountStateCache.commitRemovals();
			pHarvester->GetCurrentCache().commit(Height());

			// Sanity:
			EXPECT_EQ(0u, accountStateCache.size());
		}

		{
			auto delta = pHarvester->GetPreviousCache().createDelta();
			auto& accountStateCache = delta.sub<cache::AccountStateCache>();
			for (auto pState : context.PreviousAccountStates)
				accountStateCache.queueRemove(pState->Address, pState->AddressHeight);

			accountStateCache.commitRemovals();
			pHarvester->GetPreviousCache().commit(Height());

			// Sanity:
			EXPECT_EQ(0u, accountStateCache.size());
		}

		EXPECT_NE(0u, context.pUnlockedAccounts->view().size());

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TEST(TEST_CLASS, HarvestHasFirstHarvesterWithHitAsSigner) {
		// Arrange:
		HarvesterContext context;
		auto pHarvester = context.CreateHarvester();
		auto firstPublicKey = context.pUnlockedAccounts->view().begin()->publicKey();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		ASSERT_TRUE(!!pBlock);
		EXPECT_EQ(firstPublicKey, pBlock->Signer);
	}

	TEST(TEST_CLASS, HarvestedBlockHasExpectedProperties) {
		// Arrange:
		// - the harvester accepts the first account that has a hit. That means that subsequent accounts might have
		// - a better (lower) hit but still won't be the signer of the block.
		test::RunNonDeterministicTest("harvested block has expected properties", []() {
			HarvesterContext context;
			auto bestKey = BestHarvesterKey(context.LastBlockElement, context.KeyPairs);
			auto timestamp = CalculateBlockGenerationTime(context, bestKey);
			auto pHarvester = context.CreateHarvester();

			// Act:
			auto pBlock = pHarvester->harvest(context.LastBlockElement, timestamp);
			if (!pBlock)
				return false;

			// Assert:
			EXPECT_TRUE(!!pBlock);
			EXPECT_EQ(timestamp, pBlock->Timestamp);
			EXPECT_EQ(Height(2), pBlock->Height);
			EXPECT_EQ(model::CalculateHash(*context.pLastBlock), pBlock->PreviousBlockHash);
			EXPECT_TRUE(model::VerifyBlockHeaderSignature(*pBlock));
			EXPECT_EQ(model::MakeVersion(Network_Identifier, 3), pBlock->Version);
			EXPECT_EQ(model::Entity_Type_Block, pBlock->Type);
			EXPECT_TRUE(model::IsSizeValid(*pBlock, model::TransactionRegistry()));
			return true;
		});
	}

	// region transaction supplier

	namespace {
		TransactionsInfo CreateTransactionsInfo(size_t count) {
			TransactionsInfo info;
			for (auto i = 0u; i < count; ++i)
				info.Transactions.push_back(test::GenerateRandomTransaction());

			test::FillWithRandomData(info.TransactionsHash);
			return info;
		}

		void AssertTransactionsInBlock(
				size_t numAvailableTransactions,
				uint32_t maxTransactionsPerBlock,
				size_t numExpectedTransactionsInBlock) {
			// Arrange:
			HarvesterContext context;
			size_t counter = 0;
			uint32_t numRequestedInfos = 0;
			auto info = CreateTransactionsInfo(numAvailableTransactions);
			auto config = CreateConfiguration();
			config.MaxTransactionsPerBlock = maxTransactionsPerBlock;
			auto pHarvester = context.CreateHarvester(std::move(config), [&, info](auto count) mutable {
				++counter;
				numRequestedInfos = count;
				if (info.Transactions.size() > count)
					info.Transactions.resize(count);

				return info;
			});

			// Act:
			auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

			// Assert:
			ASSERT_TRUE(!!pBlock);
			EXPECT_EQ(1u, counter);
			EXPECT_EQ(maxTransactionsPerBlock, numRequestedInfos);
			EXPECT_EQ(info.TransactionsHash, pBlock->BlockTransactionsHash);

			size_t i = 0;
			for (const auto& transaction : pBlock->Transactions()) {
				EXPECT_EQ(*info.Transactions[i], transaction) << "transaction at " << i;
				++i;
			}

			EXPECT_EQ(numExpectedTransactionsInBlock, i);
		}
	}

	TEST(TEST_CLASS, HarvestUsesTransactionSupplier) {
		// Arrange:
		HarvesterContext context;
		size_t counter = 0u;
		auto pHarvester = context.CreateHarvester(
				CreateConfiguration(),
				[&counter](size_t) {
					++counter;
					return TransactionsInfo();
				});

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_TRUE(!!pBlock);
		EXPECT_EQ(1u, counter);
	}

	TEST(TEST_CLASS, HarvestPutsNoTransactionsInBlockIfCacheIsEmpty) {
		// Assert: numAvailableTransactions / maxTransactionsPerBlock / numExpectedTransactionsInBlock
		AssertTransactionsInBlock(0, 10, 0);
	}

	TEST(TEST_CLASS, HarvestPutsAllTransactionsFromCacheIntoBlockIfAllAreRequested) {
		// Assert: numAvailableTransactions / maxTransactionsPerBlock / numExpectedTransactionsInBlock
		AssertTransactionsInBlock(5, 10, 5);
	}

	TEST(TEST_CLASS, HarvestPutsMaxTransactionsIntoBlockIfMaxTransactionsAreRequestedAndCacheHasEnoughTransactions) {
		// Assert: numAvailableTransactions / maxTransactionsPerBlock / numExpectedTransactionsInBlock
		AssertTransactionsInBlock(10, 5, 5);
	}

	// endregion
}}
