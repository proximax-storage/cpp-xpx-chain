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

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheDelta.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/model/Block.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/Logging.h"
#include "catapult/utils/TimeSpan.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/KeyPairTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace chain {

#define TEST_CLASS BlockScorerTests

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
		EXPECT_EQ(0xF0F1F2F3F4F5F6F7u, hit);
	}

	TEST(TEST_CLASS, CanCalculateHitWhenGenerationHashIsZero) {
		// Arrange:
		const Hash256 generationHash{ {} };

		// Act:
		auto hit = CalculateHit(generationHash);

		// Assert:
		EXPECT_EQ(0u, hit);
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

	// region CalculateTarget

	TEST(TEST_CLASS, BlockTargetIsZeroWhenParentBlockTargetIsZero) {
		// Act:
		auto target = CalculateTarget(BlockTarget{0}, utils::TimeSpan::FromSeconds(15), Amount{1000});

		// Assert:
		EXPECT_EQ(BlockTarget{0}, target);
	}

	TEST(TEST_CLASS, BlockTargetIsZeroWhenElapsedTimeIsZero) {
		// Act:
		auto target = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(0), Amount{1000});

		// Assert:
		EXPECT_EQ(BlockTarget{0}, target);
	}

	TEST(TEST_CLASS, BlockTargetIncreasesAlongWithParentBlockTarget) {
		// Act:
		auto target1 = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), Amount{1000});
		auto target2 = CalculateTarget(BlockTarget{2000}, utils::TimeSpan::FromSeconds(10), Amount{1000});

		// Assert:
		EXPECT_GT(target2, target1);
	}

	TEST(TEST_CLASS, BlockTargetIncreasesAlongWithTime) {
		// Act:
		auto target1 = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), Amount{1000});
		auto target2 = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(20), Amount{1000});

		// Assert:
		EXPECT_GT(target2, target1);
	}

	TEST(TEST_CLASS, BlockTargetIncreasesAlongWithEffectiveBalance) {
		// Act:
		auto target1 = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), Amount{1000});
		auto target2 = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), Amount{2000});

		// Assert:
		EXPECT_GT(target2, target1);
	}

	TEST(TEST_CLASS, BlockTargetIsCorrectlyCalculated) {
		// Act:
		auto target = CalculateTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), Amount{1000});

		// Assert:
		EXPECT_EQ(1000 * 10 * 1000, target);
	}

	// endregion

	// region CalculateBaseTarget

	namespace {
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		model::BlockChainConfiguration CreateConfiguration(
				uint64_t blockGenerationTargetTimeSeconds = 15u,
				uint64_t blockTimeSmoothingFactor = 1000u) {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(blockGenerationTargetTimeSeconds);
			config.BlockTimeSmoothingFactor = blockTimeSmoothingFactor;
			config.Network.Identifier = Network_Identifier;
			return config;
		}
	}

	TEST(TEST_CLASS, CalculateBaseTargetAssertsIfBlockGenerationTargetTimeZero) {
		// Assert:
		EXPECT_THROW(CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), CreateConfiguration(0u, 0u)), catapult_invalid_argument);
		EXPECT_THROW(CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), CreateConfiguration(0u, 1000u)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, BaseTargetDoesntChangeIfBlockTimeSmoothingFactorZero_BlockGenerationTimeGreaterThanTargetTime) {
		// Arrange:
		auto baseTarget = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(20), CreateConfiguration(15u, 0u));

		// Assert:
		EXPECT_EQ(BlockTarget{1000}, baseTarget);
	}

	TEST(TEST_CLASS, BaseTargetDoesntChangeIfBlockTimeSmoothingFactorZero_BlockGenerationTimeLesserThanTargetTime) {
		// Arrange:
		auto baseTarget = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), CreateConfiguration(15u, 0u));

		// Assert:
		EXPECT_EQ(BlockTarget{1000}, baseTarget);
	}

	TEST(TEST_CLASS, BaseTargetDoesntChangeIfBlockGenerationTimeEqualToTargetTime) {
		// Arrange:
		auto baseTarget = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(15), CreateConfiguration(15u, 1000u));

		// Assert:
		EXPECT_EQ(BlockTarget{1000}, baseTarget);
	}

	TEST(TEST_CLASS, BaseTargetProportionalToBlockGenerationTime_BlockGenerationTimeGreaterThanTargetTime) {
		// Arrange:
		auto config = CreateConfiguration(15u, 11000u);
		auto baseTarget1 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(25), config);
		auto baseTarget2 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(20), config);

		// Assert:
		EXPECT_LT(baseTarget2, baseTarget1);
	}

	TEST(TEST_CLASS, BaseTargetProportionalToBlockGenerationTime_BlockGenerationTimeLesserThanTargetTime) {
		// Arrange:
		auto config = CreateConfiguration(15u, 11000u);
		auto baseTarget1 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), config);
		auto baseTarget2 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(5), config);

		// Assert:
		EXPECT_LT(baseTarget2, baseTarget1);
	}

	TEST(TEST_CLASS, BaseTargetPaddedBySmothingFactor_BlockGenerationTimeGreaterThanTargetTime) {
		// Arrange:
		auto config = CreateConfiguration(15u, 1000u);
		auto baseTarget1 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(25), config);
		auto baseTarget2 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(20), config);

		// Assert:
		EXPECT_EQ(baseTarget2, baseTarget1);
	}

	TEST(TEST_CLASS, BaseTargetPaddedBySmothingFactor_BlockGenerationTimeLesserThanTargetTime) {
		// Arrange:
		auto config = CreateConfiguration(15u, 1000u);
		auto baseTarget1 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(5), config);
		auto baseTarget2 = CalculateBaseTarget(BlockTarget{1000}, utils::TimeSpan::FromSeconds(10), config);

		// Assert:
		EXPECT_EQ(baseTarget2, baseTarget1);
	}

	// endregion

	// region CalculateEffectiveBalance

	namespace {
		void CreateAccount(
				cache::AccountStateCacheDelta& cache,
				const crypto::KeyPair& keyPair,
				const Amount& balance) {
			auto& accountState = cache.addAccount(keyPair.publicKey(), Height{1});
			accountState.Balances.credit(Xpx_Id, balance);
		}

		void CreateAccount(
				cache::AccountStateCacheDelta& cache,
				const Address& address,
				const Amount& balance) {
			auto& accountState = cache.addAccount(address, Height{1});
			accountState.Balances.credit(Xpx_Id, balance);
		}

		struct AccountContext {
		public:
			AccountContext(
				const Amount& currentBalance,
				const Amount& previousBalance,
				bool addAccountByAddress,
				bool populateCurrentCache = true,
				bool populatePreviousCache = true,
				model::NetworkIdentifier addressNetworkIdentifier = Network_Identifier)
					: CurrentCache(test::CreateEmptyCatapultCache(CreateConfiguration()))
					, PreviousCache(test::CreateEmptyCatapultCache(CreateConfiguration()))
					, KeyPair(test::GenerateKeyPair()) {

				if (populateCurrentCache) {
					auto delta = CurrentCache.createDelta();
					if (addAccountByAddress) {
						CreateAccount(delta.sub<cache::AccountStateCache>(),
							model::PublicKeyToAddress(KeyPair.publicKey(), addressNetworkIdentifier), currentBalance);
					} else {
						CreateAccount(delta.sub<cache::AccountStateCache>(), KeyPair, currentBalance);
					}
					CurrentCache.commit(Height());
				}

				if (populatePreviousCache) {
					auto delta = PreviousCache.createDelta();
					if (addAccountByAddress) {
						CreateAccount(delta.sub<cache::AccountStateCache>(),
							model::PublicKeyToAddress(KeyPair.publicKey(), addressNetworkIdentifier), previousBalance);
					} else {
						CreateAccount(delta.sub<cache::AccountStateCache>(), KeyPair, previousBalance);
					}
					PreviousCache.commit(Height());
				}
			}

		public:
			cache::CatapultCache CurrentCache;
			cache::CatapultCache PreviousCache;
			crypto::KeyPair KeyPair;
		};
	}

	TEST(TEST_CLASS, ReturnedPreviousBalance_HeightsEqual_PublicKey) {
		// Arrange:
		AccountContext context{Amount{100}, Amount{10}, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{100}, balance);
	}

	TEST(TEST_CLASS, ReturnedPreviousBalance_HeightGreater_PublicKey) {
		// Arrange:
		AccountContext context{Amount{100}, Amount{10}, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedPreviousBalance_HeightsEqual_Address) {
		// Arrange:
		AccountContext context{Amount{100}, Amount{10}, true};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{100}, balance);
	}

	TEST(TEST_CLASS, ReturnedPreviousBalance_HeightGreater_Address) {
		// Arrange:
		AccountContext context{Amount{100}, Amount{10}, true};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_HeightLesser_PublicKey) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{2},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_HeightsEqual_PublicKey) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_HeightGreater_PublicKey) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_HeightLesser_Address) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, true};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{2},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_HeightsEqual_Address) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, true};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_HeightGreater_Address) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, true};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_CurrentBalanceNotPopulated_HeightLesser) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{2},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_CurrentBalanceNotPopulated_HeightsEqual) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_CurrentBalanceNotPopulated_HeightGreater) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_PreviousBalanceNotPopulated_HeightLesser) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false, true, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{2},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedCurrentBalance_PreviousBalanceNotPopulated_HeightsEqual) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false, true, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{10}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_PreviousBalanceNotPopulated_HeightGreater) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false, true, false};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_WrongPublicKey_HeightLesser) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false};
		auto publicKey = context.KeyPair.publicKey();
		publicKey[0] = ~publicKey[0];

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{2},
			Height{1},
			publicKey);

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_WrongPublicKey_HeightsEqual) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false};
		auto publicKey = context.KeyPair.publicKey();
		publicKey[0] = ~publicKey[0];

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			publicKey);

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_WrongPublicKey_HeightGreater) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, false};
		auto publicKey = context.KeyPair.publicKey();
		publicKey[0] = ~publicKey[0];

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			publicKey);

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedXeroBalance_WrongNetworkIdentifier_HeightLesser) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, true, true, true, model::NetworkIdentifier::Public_Test};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{2},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_WrongNetworkIdentifier_HeightsEqual) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, true, true, true, model::NetworkIdentifier::Public_Test};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{1},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	TEST(TEST_CLASS, ReturnedZeroBalance_WrongNetworkIdentifier_HeightGreater) {
		// Arrange:
		AccountContext context{Amount{10}, Amount{100}, true, true, true, model::NetworkIdentifier::Public_Test};

		// Act:
		auto balance = CalculateEffectiveBalance(
			cache::ReadOnlyAccountStateCache(context.CurrentCache.createView().sub<cache::AccountStateCache>()),
			cache::ReadOnlyAccountStateCache(context.PreviousCache.createView().sub<cache::AccountStateCache>()),
			Height{1},
			Height{2},
			context.KeyPair.publicKey());

		// Assert:
		EXPECT_EQ(Amount{0}, balance);
	}

	// endregion

	// region BlockHitPredicate

	TEST(TEST_CLASS, BlockHitPredicateReturnsTrueWhenHitIsLessThanTarget) {
		// Arrange:
		const Hash256 generationHash{ { 0xF7 } };
		const BlockTarget baseTarget{100};
		const utils::TimeSpan elapsedTime{utils::TimeSpan::FromSeconds(2)};
		const Amount effectiveBalance{10};

		BlockHitPredicate predicate;

		// Act:
		auto isHit = predicate(generationHash, baseTarget, elapsedTime, effectiveBalance);

		// Assert:
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(baseTarget, elapsedTime, effectiveBalance);
		EXPECT_LT(hit, target);
		EXPECT_TRUE(isHit);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsTrueWhenHitIsLessThanTarget_Context) {
		// Arrange:
		model::BlockHitContext hitContext;
		hitContext.GenerationHash = Hash256{ { 0xF7 } };
		hitContext.BaseTarget = BlockTarget{100};
		hitContext.ElapsedTime = utils::TimeSpan::FromSeconds(2);
		hitContext.EffectiveBalance = Amount{10};

		BlockHitPredicate predicate;

		// Act:
		auto isHit = predicate(hitContext);

		// Assert:
		auto hit = CalculateHit(hitContext.GenerationHash);
		auto target = CalculateTarget(hitContext.BaseTarget, hitContext.ElapsedTime, hitContext.EffectiveBalance);
		EXPECT_LT(hit, target);
		EXPECT_TRUE(isHit);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsEqualToTarget) {
		// Arrange:
		const Hash256 generationHash{ { 0xC8 } };
		const BlockTarget baseTarget{10};
		const utils::TimeSpan elapsedTime{utils::TimeSpan::FromSeconds(2)};
		const Amount effectiveBalance{10};

		BlockHitPredicate predicate;

		// Act:
		auto isHit = predicate(generationHash, baseTarget, elapsedTime, effectiveBalance);

		// Assert:
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(baseTarget, elapsedTime, effectiveBalance);
		EXPECT_EQ(hit, target);
		EXPECT_FALSE(isHit);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsEqualToTarget_Context) {
		// Arrange:
		model::BlockHitContext hitContext;
		hitContext.GenerationHash = Hash256{ { 0xC8 } };
		hitContext.BaseTarget = BlockTarget{10};
		hitContext.ElapsedTime = utils::TimeSpan::FromSeconds(2);
		hitContext.EffectiveBalance = Amount{10};

		BlockHitPredicate predicate;

		// Act:
		auto isHit = predicate(hitContext);

		// Assert:
		auto hit = CalculateHit(hitContext.GenerationHash);
		auto target = CalculateTarget(hitContext.BaseTarget, hitContext.ElapsedTime, hitContext.EffectiveBalance);
		EXPECT_EQ(hit, target);
		EXPECT_FALSE(isHit);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsGreaterThanTarget) {
		// Arrange:
		const Hash256 generationHash{ { 0xF7 } };
		const BlockTarget baseTarget{10};
		const utils::TimeSpan elapsedTime{utils::TimeSpan::FromSeconds(2)};
		const Amount effectiveBalance{10};

		BlockHitPredicate predicate;

		// Act:
		auto isHit = predicate(generationHash, baseTarget, elapsedTime, effectiveBalance);

		// Assert:
		auto hit = CalculateHit(generationHash);
		auto target = CalculateTarget(baseTarget, elapsedTime, effectiveBalance);
		EXPECT_GT(hit, target);
		EXPECT_FALSE(isHit);
	}

	TEST(TEST_CLASS, BlockHitPredicateReturnsFalseWhenHitIsGreaterThanTarget_Context) {
		// Arrange:
		model::BlockHitContext hitContext;
		hitContext.GenerationHash = Hash256{ { 0xF7 } };
		hitContext.BaseTarget = BlockTarget{10};
		hitContext.ElapsedTime = utils::TimeSpan::FromSeconds(2);
		hitContext.EffectiveBalance = Amount{10};

		BlockHitPredicate predicate;

		// Act:
		auto isHit = predicate(hitContext);

		// Assert:
		auto hit = CalculateHit(hitContext.GenerationHash);
		auto target = CalculateTarget(hitContext.BaseTarget, hitContext.ElapsedTime, hitContext.EffectiveBalance);
		EXPECT_GT(hit, target);
		EXPECT_FALSE(isHit);
	}

	// endregion
}}
