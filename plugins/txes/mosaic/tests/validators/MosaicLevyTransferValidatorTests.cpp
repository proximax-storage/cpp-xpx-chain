/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/BalanceTransfers.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "src/validators/Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/MosaicTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/LevyTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicLevyTransferValidatorTests
		
	DEFINE_COMMON_VALIDATOR_TESTS(LevyTransfer, )
		
		/// region helper functions
		namespace {
			auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
			using LevySetupFunc = std::function<void (cache::CatapultCacheDelta&, const Key&)>;
			
			void AssertLevyParameterTest(ValidationResult expectedResult, cache::CatapultCache& cache, LevySetupFunc action,
				const config::BlockchainConfiguration& config, const Height height = Height(1)) {
				
				auto delta = cache.createDelta();
				auto signer = test::GenerateRandomByteArray<Key>();
				
				auto notification = model::BalanceTransferNotification<1>(signer,
					UnresolvedAddress(),test::UnresolveXor(Currency_Mosaic_Id), Amount(123));
				
				action(delta, signer);
				cache.commit(Height());
				
				auto pValidator = CreateLevyTransferValidator();
				
				auto result = test::ValidateNotification(*pValidator, notification, cache, config, height);
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
		}
		/// end region
		
		// region validator tests
		
		TEST(TEST_CLASS, LevyConfigDisabledWithNoLevy) {
			auto config = test::CreateMosaicConfigWithLevy(false);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(ValidationResult::Success, cache, [](cache::CatapultCacheDelta&, const Key&) {}, config);
		}
		
		TEST(TEST_CLASS, LevyConfigEnabledWithNoLevy) {
			auto config = test::CreateMosaicConfigWithLevy(true);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(ValidationResult::Success, cache, [](cache::CatapultCacheDelta&, const Key&) {}, config);
		}
		
		TEST(TEST_CLASS, LevyConfigEnabledWithLevyDeleted) {
			auto config = test::CreateMosaicConfigWithLevy(true);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(ValidationResult::Success, cache, [](cache::CatapultCacheDelta& cache, const Key&) {
				auto& levyCacheDelta = cache.sub<cache::LevyCache>();
				auto levyEntry = test::CreateLevyEntry(false, false);
				levyCacheDelta.insert(levyEntry);
			}, config);
		}
		
		TEST(TEST_CLASS, LevyConfigEnabledLevyNotFound) {
			auto config = test::CreateMosaicConfigWithLevy(true);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(Failure_Mosaic_Levy_Not_Found_Or_Expired, cache, [](cache::CatapultCacheDelta& cache, const Key&) {
				auto& levyCacheDelta = cache.sub<cache::LevyCache>();
				auto levy = test::CreateLevyEntry(Currency_Mosaic_Id, Amount(10));
				auto levyEntry = test::CreateLevyEntry(Currency_Mosaic_Id, levy, true, false);
				levyCacheDelta.insert(levyEntry);
			}, config);
		}
		
		TEST(TEST_CLASS, LevyConfigEnabledLevyMosaicExpired) {
			auto config = test::CreateMosaicConfigWithLevy(true);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(Failure_Mosaic_Levy_Not_Found_Or_Expired, cache, [](cache::CatapultCacheDelta& cache, const Key&) {
				auto levy = test::CreateLevyEntry(MosaicId(333), Amount(1000));
				
				// Crete Mosaic at Height 1 with duration 10, validate at height 100
				test::AddMosaic(cache, levy.MosaicId, Height(1), BlockDuration(10), Key());
				test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levy, Key());
				
			}, config, Height(100));
		}
		
		TEST(TEST_CLASS, LevyConfigNotEnoughBalance) {
			auto config = test::CreateMosaicConfigWithLevy(true);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(Failure_Mosaic_Insufficient_Levy_Balance, cache, [](cache::CatapultCacheDelta& cache, const Key&) {
				auto levy = test::CreateLevyEntry(MosaicId(333), Amount(1000));
				
				test::AddMosaic(cache, levy.MosaicId, Height(1), Eternal_Artifact_Duration, Key());
				test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levy, Key());
				
			}, config);
		}
		
		TEST(TEST_CLASS, LevyValidationSuccess) {
			auto config = test::CreateMosaicConfigWithLevy(true);
			auto cache = test::LevyCacheFactory::Create(config);
			AssertLevyParameterTest(ValidationResult::Success, cache, [](cache::CatapultCacheDelta& cache, const Key& sender) {
				auto levy = test::CreateLevyEntry(MosaicId(333), Amount(1000));
				
				test::AddMosaic(cache, levy.MosaicId, Height(1), Eternal_Artifact_Duration, Key());
				test::AddMosaicWithLevy(cache, Currency_Mosaic_Id, Height(1), levy, Key());
				test::SetCacheBalances(cache, sender, test::CreateMosaicBalance( levy.MosaicId, Amount(1000)));
				
			}, config);
		}
	}}
