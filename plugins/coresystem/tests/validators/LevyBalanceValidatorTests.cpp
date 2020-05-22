/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/types.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS LevyBalanceValidatorTests
	DEFINE_COMMON_VALIDATOR_TESTS(LevyBalance,)
		
	namespace {
		auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
		auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
		auto Levy_Mosaic_Id = MosaicId(12);
		
		model::ResolverContext CreateLevyResolverContext() {
			return model::ResolverContext(
				[](const auto&) { return MosaicId(1); },
				[](const auto&) { return Address(); },
				[](const auto&) { return Amount(1000); },
				[](const auto&) { return Levy_Mosaic_Id; },
				[](const auto&) { return test::GenerateRandomByteArray<Address>(); });
		}
		
		void RunTest( ValidationResult expectedResult, Amount beginBalance, MosaicId mosaicBalance) {
			auto cache = test::CreateEmptyCatapultCache();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();
			
			test::SetCacheBalances(delta, signer, { { mosaicBalance, beginBalance } });
			cache.commit(Height());
			
			/// create unresolved data
			auto notification = model::LevyTransferNotification<1>(signer, UnresolvedLevyAddress(),
				UnresolvedLevyMosaicId(Unresolved_Mosaic_Id.unwrap()), UnresolvedAmount(Amount(2000000)));
			
			auto resolverContext = CreateLevyResolverContext( );
			
			// Arrange:
			auto pValidator = CreateLevyBalanceValidator();
			
			auto validatorContext = ValidatorContext(config::BlockchainConfiguration::Uninitialized(),
				Height(1), Timestamp(0), resolverContext, delta.toReadOnly());
			
			auto result = test::ValidateNotification(*pValidator, notification, validatorContext);
			
			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}
	
	TEST(TEST_CLASS, LevyInsufficientBalance) {
		RunTest(Failure_Core_Insufficient_Balance, Amount(100), Levy_Mosaic_Id);
	}
		
	TEST(TEST_CLASS, LevyNoBalanceForSpecificMosaic) {
		RunTest(Failure_Core_Insufficient_Balance, Amount(100), MosaicId(1051));
	}
	
	TEST(TEST_CLASS, LevyEnoughBalance) {
		RunTest(ValidationResult::Success, Amount(10000), Levy_Mosaic_Id);
	}
}}
