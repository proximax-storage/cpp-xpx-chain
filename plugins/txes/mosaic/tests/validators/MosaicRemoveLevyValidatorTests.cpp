/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/LevyTestUtils.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "catapult/constants.h"

namespace catapult {
	namespace validators {

#define TEST_CLASS MosaicRemoveLevyValidatorTests
		
		DEFINE_COMMON_VALIDATOR_TESTS(RemoveLevy,)
		
		/// region helper functions
		namespace {
			auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
			
			/// This itself looks like a common function, all Notification inherit from Notification object
			void AssertValidationResult(ValidationResult expectedResult, const MosaicId& mosaicId, const Key& signer,
			                            cache::CatapultCache &cache) {
				
				// Arrange:
				auto pValidator = CreateRemoveLevyValidator();
				auto notification = model::MosaicRemoveLevyNotification<1>(test::UnresolveXor(mosaicId), signer);
				
				auto result = test::ValidateNotification(*pValidator, notification, cache);
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
		}
		/// end region
		
		// region validator tests
		TEST(TEST_CLASS, RemoveLevyOk) {
			auto cache = test::LevyCacheFactory::Create();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = test::CreateValidMosaicLevy();
			
			/// add the base mosaic where we will add the levy
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), levy, signer);
			cache.commit(Height());
			
			AssertValidationResult(ValidationResult::Success, Currency_Mosaic_Id, signer, cache);
		}
		
		TEST(TEST_CLASS, IneligibleSigner) {
			auto cache = test::LevyCacheFactory::Create();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto owner = test::GenerateRandomByteArray<Key>();
			auto levy = test::CreateValidMosaicLevy();
			
			/// add the base mosaic where we will add the levy
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), levy, owner);
			cache.commit(Height());
			
			AssertValidationResult(Failure_Mosaic_Ineligible_Signer, Currency_Mosaic_Id, signer, cache);
		}
		
		TEST(TEST_CLASS, MosaicLevyNotFound) {
			auto cache = test::LevyCacheFactory::Create();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();

			test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, signer);
			cache.commit(Height());
			
			AssertValidationResult(Failure_Mosaic_Levy_Not_Found, Currency_Mosaic_Id, signer, cache);
		}
		
		TEST(TEST_CLASS, MosaicIdNotFound) {
			auto cache = test::LevyCacheFactory::Create();
			auto signer = test::GenerateRandomByteArray<Key>();
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, Currency_Mosaic_Id, signer, cache);
		}
		
		// endregion
	}
}
