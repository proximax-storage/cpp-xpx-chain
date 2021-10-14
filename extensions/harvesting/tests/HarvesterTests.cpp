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

#include <catapult/utils/SignatureVersionToKeyTypeResolver.h>
#include "harvesting/src/Harvester.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/model/Block.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/TransactionPlugin.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/nodeps/Waits.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include "catapult/constants.h"

using catapult::crypto::KeyPair;

namespace catapult { namespace harvesting {

#define TEST_CLASS HarvesterTests

	namespace {
		// region constants / factory functions

		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		constexpr Timestamp Max_Time(std::numeric_limits<int64_t>::max());
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);
		constexpr size_t Num_Accounts = 5;

		std::vector<KeyPair> CreateKeyPairs(size_t count, uint32_t defaultAccountVersion) {
			std::vector<KeyPair> keyPairs;
			for (auto i = 0u; i < count; ++i)
			{
				if(i == 0)
					keyPairs.push_back(test::GenerateKeyPair(defaultAccountVersion));
				//All additional unlockedAccounts have to be V2
				else
					keyPairs.push_back(test::GenerateKeyPair(2));
			}
			return keyPairs;
		}
		std::vector<state::AccountState*> CreateAccounts(
				cache::AccountStateCacheDelta& cache,
				const std::vector<KeyPair>& keyPairs,
				const std::vector<KeyPair>& vrfKeyPairs) {
			std::vector<state::AccountState*> accountStates;
			for (auto i = 0u; i < keyPairs.size(); ++i) {
				cache.addAccount(keyPairs[i].publicKey(), Height(1), 2);
				auto& accountState = cache.find(keyPairs[i].publicKey()).get();
				accountState.SupplementalPublicKeys.vrf().set(vrfKeyPairs[i].publicKey());
				auto& balances = accountState.Balances;
				balances.credit(Harvesting_Mosaic_Id, Amount(1'000'000'000'000'000), Height(1));
				balances.track(Harvesting_Mosaic_Id);
				accountStates.push_back(&accountState);
			}

			return accountStates;
		}

		model::UniqueEntityPtr<model::Block> CreateBlock() {
			// the created block needs to have height 1 to be able to add it to the block difficulty cache
			auto pBlock = test::GenerateEmptyRandomBlock();
			pBlock->Height = Height(1);
			pBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);
			pBlock->Timestamp = Timestamp();
			pBlock->FeeInterest = 1;
			pBlock->FeeInterestDenominator = 2;
			return pBlock;
		}

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;

			config.Immutable.NetworkIdentifier = Network_Identifier;

			config.Network.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15);
			config.Network.BlockTimeSmoothingFactor = 0;
			config.Network.MaxDifficultyBlocks = 60;
			config.Network.ImportanceGrouping = 123;
			config.Network.TotalChainImportance = test::Default_Total_Chain_Importance;
			config.Network.GreedDelta = 0.5;
			config.Network.GreedExponent = 2.0;
			config.Network.AccountVersion = 2; // In this test we assume version 2 accounts are enabled otherwise there can only be one harvesting account per node, the configured one.
			config.Node.FeeInterest = 1;
			config.Node.FeeInterestDenominator = 2;

			return config.ToConst();
		}

		// endregion


		// region HarvesterContext
		template<uint32_t TBaseAccountVersion>
		struct HarvesterContext {
		public:
			HarvesterContext()
					: Config(CreateConfiguration())
					, Cache(test::CreateEmptyCatapultCache(Config))
					, KeyPairs(CreateKeyPairs(Num_Accounts, TBaseAccountVersion)) // additional keypairs are always V2
					, VrfKeyPairs(CreateKeyPairs(Num_Accounts, 2))
					, Beneficiary(test::GenerateRandomByteArray<Key>())
					, pUnlockedAccounts(std::make_unique<UnlockedAccounts>(Num_Accounts, [](const auto&) { return 0; }))
					, pLastBlock(CreateBlock())
					, LastBlockElement(test::BlockToBlockElement(*pLastBlock)) {
				auto delta = Cache.createDelta();
				auto &accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				AccountStates = CreateAccounts(accountStateCacheDelta, KeyPairs, VrfKeyPairs);

				auto& difficultyCache = delta.sub<cache::BlockDifficultyCache>();
				state::BlockDifficultyInfo info(pLastBlock->Height, pLastBlock->Timestamp, pLastBlock->Difficulty);
				difficultyCache.insert(info);
				Cache.commit(Height(1));
				UnlockAllAccounts(*pUnlockedAccounts, KeyPairs, VrfKeyPairs);

				LastBlockElement.GenerationHash = test::GenerateRandomByteArray<GenerationHash>();
			}

		public:
			std::unique_ptr<Harvester> CreateHarvester() {
				return CreateHarvester(Config);
			}

			std::unique_ptr<Harvester> CreateHarvester(const config::BlockchainConfiguration& config) {
				return CreateHarvester(config, [](const auto& blockHeader, auto) {
					auto pBlock = utils::MakeUniqueWithSize<model::Block>(sizeof(model::BlockHeaderV4));
					std::memcpy(static_cast<void*>(pBlock.get()), &blockHeader, sizeof(model::BlockHeaderV4));
					return pBlock;
				});
			}

			std::unique_ptr<Harvester> CreateHarvester(
					const config::BlockchainConfiguration& config,
					const BlockGenerator& blockGenerator) {
				auto pConfigHolder = config::CreateMockConfigurationHolder(config);
				return std::make_unique<Harvester>(Cache, pConfigHolder, Beneficiary, *pUnlockedAccounts, blockGenerator);
			}

		private:
			static void UnlockAllAccounts(
				UnlockedAccounts& unlockedAccounts,
				const std::vector<KeyPair>& signingKeyPairs,
				const std::vector<KeyPair>& vrfKeyPairs) {
					auto modifier = unlockedAccounts.modifier();
					for (auto i = 0u; i < Num_Accounts; ++i) {
						modifier.add(BlockGeneratorAccountDescriptor(
								test::CopyKeyPair(signingKeyPairs[i]),
								test::CopyKeyPair(vrfKeyPairs[i]), signingKeyPairs[i].hashingType() == KeyHashingType::Sha2 ? 2 : 1));
					}
			}

		public:
			config::BlockchainConfiguration Config;
			cache::CatapultCache Cache;
			std::vector<KeyPair> KeyPairs;
			std::vector<KeyPair> VrfKeyPairs;
			Key Beneficiary;
			std::vector<state::AccountState*> AccountStates;
			std::unique_ptr<UnlockedAccounts> pUnlockedAccounts;
			std::shared_ptr<model::Block> pLastBlock;
			model::BlockElement LastBlockElement;

		public:
			std::pair<Key, uint32_t> BestHarvesterPublicKey() {
				std::pair<Key, uint32_t> pBestKeyPair;
				uint64_t bestHit = std::numeric_limits<uint64_t>::max();
				EXPECT_EQ(AccountStates.size(), KeyPairs.size());

				for (auto i = 0; i < AccountStates.size(); ++i) {
					auto account = AccountStates[i];
					auto FindPublicKeyPredicate = [&key = account->PublicKey](const KeyPair& x) {
					  return key == x.publicKey();
					};
					GenerationHash genHash;
					if(account->GetVersion() > 1)
					{
						auto vrfProof = crypto::GenerateVrfProof<Vrf_Key_Hashing_Type>(LastBlockElement.GenerationHash, VrfKeyPairs[i]);
						genHash = model::CalculateGenerationHashVrf(vrfProof.Gamma);
					}
					else
					{
						genHash = model::CalculateGenerationHash(LastBlockElement.GenerationHash, account->PublicKey);
					}
					uint64_t hit = chain::CalculateHit(genHash);
					if (hit < bestHit) {
						bestHit = hit;
						pBestKeyPair = std::make_pair((*find_if(KeyPairs.begin(), KeyPairs.end(), FindPublicKeyPredicate)).publicKey(), account->GetVersion());
					}

				}

				return pBestKeyPair;
			}
			Timestamp CalculateBlockGenerationTime(const Key& publicKey) {
				auto difficulty = chain::CalculateDifficulty(
						Cache.template sub<cache::BlockDifficultyCache>(),
						state::BlockDifficultyInfo(pLastBlock->Height + Height(1), pLastBlock->Timestamp, Difficulty()),
						Config.Network
				);

				const auto& accountStateCache = Cache.template sub<cache::AccountStateCache>();
				auto view = accountStateCache.createView(Height{0});
				cache::ReadOnlyAccountStateCache readOnlyCache(*view);
				auto account = readOnlyCache.find(publicKey).get();
				cache::ImportanceView importanceView(readOnlyCache);
				GenerationHash genHash;
				if(account.GetVersion() > 1)
				{
					auto it = find_if(KeyPairs.begin(), KeyPairs.end(), [&publicKey](const KeyPair& x) {
					  return publicKey == x.publicKey();
					});
					auto vrfProof = crypto::GenerateVrfProof<Vrf_Key_Hashing_Type>(LastBlockElement.GenerationHash, VrfKeyPairs[it - KeyPairs.begin()]);
					genHash = model::CalculateGenerationHashVrf(vrfProof.Gamma);
				}
				else genHash = model::CalculateGenerationHash(LastBlockElement.GenerationHash, publicKey);
				uint64_t hit = chain::CalculateHit(genHash);
				uint64_t referenceTarget = static_cast<uint64_t>(chain::CalculateTarget(
						utils::TimeSpan::FromMilliseconds(1000),
						difficulty,
						importanceView.getAccountImportanceOrDefault(publicKey, pLastBlock->Height),
						Config.Network,
						1, 2));
				uint64_t seconds = hit / referenceTarget;
				return Timestamp((seconds + 1) * 1000);
			}
		};


		// endregion

		// region test utils


		// endregion
	}

	// region basic tests
	namespace {
		using test_types = ::testing::Types<
				std::integral_constant<uint32_t,1>,
				std::integral_constant<uint32_t,2>>;
		template<typename TBaseAccountVersion>
		struct HarvesterTest : public ::testing::Test {};

	}



	TYPED_TEST_CASE(HarvesterTest, test_types);

	TYPED_TEST(HarvesterTest, HarvestReturnsNullptrWhenNoAccountIsUnlocked) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
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

	TYPED_TEST(HarvesterTest, HarvestReturnsBlockWhenEnoughTimeElapsed) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_TRUE(!!pBlock);
	}

	TYPED_TEST(HarvesterTest, HarvestReturnsNullptrWhenDifficultyCacheDoesNotContainInfoAtLastBlockHeight) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		auto pHarvester = context.CreateHarvester();
		auto numBlocks = context.Config.Network.MaxDifficultyBlocks + 10;

		// - seed the block difficulty cache (it already has an entry for height 1)
		{
			auto delta = context.Cache.createDelta();
			auto& blockDifficultyCache = delta.template sub<cache::BlockDifficultyCache>();
			for (auto i = 2u; i <= numBlocks; ++i)
				blockDifficultyCache.insert(state::BlockDifficultyInfo(Height(i), Timestamp(i * 1'000), Difficulty(NEMESIS_BLOCK_DIFFICULTY)));

			context.Cache.commit(Height(numBlocks));
		}

		// Act: set the last block to the max difficulty info in the cache and harvest
		context.pLastBlock->Height = Height(numBlocks);
		auto pBlock1 = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// - set the last block to one past the max difficulty info in the cache and harvest
		context.pLastBlock->Height = Height(numBlocks + 1);
		auto pBlock2 = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert: only the first block could be harvested (there is insufficient difficulty info for the second one)
		EXPECT_TRUE(!!pBlock1);
		EXPECT_FALSE(!!pBlock2);
	}

	TYPED_TEST(HarvesterTest, HarvesterWithBestKeyCreatesBlockAtEarliestMoment) {
		// Arrange:
		// - the harvester accepts the first account that has a hit. That means that subsequent accounts might have
		// - a better (lower) hit but still won't be the signer of the block.
		test::RunNonDeterministicTest("harvester with best key harvests", []() {
			HarvesterContext<TypeParam::value> context;
			auto bestKey = context.BestHarvesterPublicKey();
			auto timestamp = context.CalculateBlockGenerationTime(bestKey.first);
			auto tooEarly = Timestamp(timestamp.unwrap() - 1000);
			auto pHarvester = context.CreateHarvester();

			// Sanity: harvester cannot harvest a block before it's her turn
			auto pBlock1 = pHarvester->harvest(context.LastBlockElement, tooEarly);

			// Act: harvester should succeed at earliest possible time
			auto pBlock2 = pHarvester->harvest(context.LastBlockElement, timestamp);
			if (!pBlock2 || bestKey.first != pBlock2->Signer)
				return false;

			// Assert:
			EXPECT_FALSE(!!pBlock1);
			EXPECT_TRUE(!!pBlock2);
			EXPECT_EQ(bestKey.first, pBlock2->Signer);
			return true;
		});
	}

	TYPED_TEST(HarvesterTest, HarvestReturnsNullptrWhenNoHarvesterHasHit) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		auto bestKey = context.BestHarvesterPublicKey();
		auto timestamp = context.CalculateBlockGenerationTime(bestKey.first);
		auto tooEarly = Timestamp(timestamp.unwrap() - 1000);
		auto pHarvester = context.CreateHarvester();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, tooEarly);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TYPED_TEST(HarvesterTest, HarvestReturnsNullptrWhenAccountsAreUnlockedButNotFoundInCache) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		auto pHarvester = context.CreateHarvester();

		{
			auto cacheDelta = context.Cache.createDelta();
			auto& accountStateCache = cacheDelta.template sub<cache::AccountStateCache>();
			for (const auto& keyPair : context.KeyPairs)
				accountStateCache.queueRemove(keyPair.publicKey(), Height(1));

			accountStateCache.commitRemovals();
			context.Cache.commit(Height());

			// Sanity:
			EXPECT_EQ(0u, accountStateCache.size());
		}

		EXPECT_NE(0u, context.pUnlockedAccounts->view().size());

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		EXPECT_FALSE(!!pBlock);
	}

	TYPED_TEST(HarvesterTest, HarvestHasFirstHarvesterWithHitAsSigner) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		auto pHarvester = context.CreateHarvester();
		auto firstPublicKey = std::get<0>(*context.pUnlockedAccounts->view().begin()).signingKeyPair().publicKey();

		// Act:
		auto pBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert:
		ASSERT_TRUE(!!pBlock);
		EXPECT_EQ(firstPublicKey, pBlock->Signer);
	}

	TYPED_TEST(HarvesterTest, HarvestedBlockHasExpectedProperties) {
		// Arrange:
		// - the harvester accepts the first account that has a hit. That means that subsequent accounts might have
		// - a better (lower) hit but still won't be the signer of the block.
		test::RunNonDeterministicTest("harvested block has expected properties", []() {
			HarvesterContext<TypeParam::value> context;
			auto pLastBlock = context.pLastBlock;
			auto bestKey = context.BestHarvesterPublicKey();
			auto timestamp = context.CalculateBlockGenerationTime(bestKey.first);
			auto pHarvester = context.CreateHarvester();
			const auto& difficultyCache = context.Cache.template sub<cache::BlockDifficultyCache>();

			// Act:
			auto pBlock = pHarvester->harvest(context.LastBlockElement, timestamp);
			if (!pBlock || bestKey.first != pBlock->Signer)
				return false;

			// Assert:
			EXPECT_TRUE(!!pBlock);
			EXPECT_EQ(timestamp, pBlock->Timestamp);
			EXPECT_EQ(Height(2), pBlock->Height);
			EXPECT_EQ(bestKey.first, pBlock->Signer);
			EXPECT_EQ(model::CalculateHash(*context.pLastBlock), pBlock->PreviousBlockHash);
			EXPECT_TRUE(model::VerifyBlockHeaderSignature(*pBlock));
			EXPECT_EQ(chain::CalculateDifficulty(difficultyCache, state::BlockDifficultyInfo(*pBlock), context.Config.Network), pBlock->Difficulty);
			EXPECT_EQ(model::MakeVersion(Network_Identifier, bestKey.second, 4), pBlock->Version); //For now signature version and key version are the same.
			EXPECT_EQ(model::Entity_Type_Block, pBlock->Type);
			EXPECT_TRUE(model::IsSizeValid(*pBlock, model::TransactionRegistry()));
			EXPECT_EQ(context.Beneficiary, pBlock->Beneficiary);
			return true;
		});
	}

	// endregion

	// region block generator delegation

	namespace {
		bool IsAnyKeyPairMatch(const std::vector<KeyPair>& keyPairs, const Key& key) {
			return std::any_of(keyPairs.cbegin(), keyPairs.cend(), [&key](const auto& keyPair) {
				return key == keyPair.publicKey();
			});
		}
	}

		TYPED_TEST(HarvesterTest, HarvestDelegatesToBlockGenerator) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		const_cast<uint32_t&>(context.Config.Network.MaxTransactionsPerBlock) = 123;
		std::vector<std::pair<Key, uint32_t>> capturedParams;
		auto pHarvester = context.CreateHarvester(context.Config, [&capturedParams](const auto& blockHeader, auto maxTransactionsPerBlock) {
			capturedParams.emplace_back(blockHeader.Signer, maxTransactionsPerBlock);
			auto pBlock = test::GenerateEmptyRandomBlock();
			pBlock->Signer = blockHeader.Signer;
			return pBlock;
		});

		// Act:
		auto pHarvestedBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert: properly signed valid block was harvested
		ASSERT_TRUE(!!pHarvestedBlock);
		EXPECT_TRUE(model::VerifyBlockHeaderSignature(*pHarvestedBlock));

		// - generator was called with expected params
		ASSERT_EQ(1u, capturedParams.size());
		EXPECT_TRUE(IsAnyKeyPairMatch(context.KeyPairs, capturedParams[0].first));
		EXPECT_EQ(123u, capturedParams[0].second);

		// - block signer was passed to generator
		EXPECT_EQ(capturedParams[0].first, pHarvestedBlock->Signer);
	}

	TYPED_TEST(HarvesterTest, HarvestReturnsNullptrWhenBlockGeneratorFails) {
		// Arrange:
		HarvesterContext<TypeParam::value> context;
		const_cast<uint32_t&>(context.Config.Network.MaxTransactionsPerBlock) = 123;
		std::vector<std::pair<Key, uint32_t>> capturedParams;
		auto pHarvester = context.CreateHarvester(context.Config, [&capturedParams](const auto& blockHeader, auto maxTransactionsPerBlock) {
			capturedParams.emplace_back(blockHeader.Signer, maxTransactionsPerBlock);
			return nullptr;
		});

		// Act:
		auto pHarvestedBlock = pHarvester->harvest(context.LastBlockElement, Max_Time);

		// Assert: no block was harvested
		EXPECT_FALSE(!!pHarvestedBlock);

		// - generator was called with expected params
		ASSERT_EQ(1u, capturedParams.size());
		EXPECT_TRUE(IsAnyKeyPairMatch(context.KeyPairs, capturedParams[0].first));
		EXPECT_EQ(123u, capturedParams[0].second);
	}

	// endregion
}}
