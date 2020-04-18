/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "tests/test/LevyTestUtils.h"

namespace catapult {
	namespace validators {

#define TEST_CLASS MosaicUpdateLevyValidatorTests

		DEFINE_COMMON_VALIDATOR_TESTS(UpdateLevy,)

		namespace {
			
			auto Currency_Mosaic_Id = MosaicId(1234);
			
			model::MosaicLevy CreateTestMosaicLevy(const MosaicId& id = Currency_Mosaic_Id,
				Amount fee = Amount(10)) {
				
				auto levy = model::MosaicLevy();
				levy.Type = model::LevyType::Absolute;
				levy.MosaicId = id;
				levy.Fee = fee;
				return levy;
			}
			
			void AssertValidationResult(ValidationResult expectedResult, model::MosaicUpdateLevyNotification<1> notification,
			                       cache::CatapultCache &cache) {
				// Arrange:
				auto pValidator = CreateUpdateLevyValidator();

				// Act:
				auto result = test::ValidateNotification(*pValidator, notification, cache);
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
			
			void AssertLevyParameters(ValidationResult expectedResult, const model::MosaicLevy& levy, uint32_t updateFlag, bool validSigner)
			{
				auto cache = test::LevyCacheFactory::Create();
				auto delta = cache.createDelta();
				auto owner = test::GenerateRandomByteArray<Key>();
				auto signer = test::GenerateRandomByteArray<Key>();
				
				auto notification = model::MosaicUpdateLevyNotification<1>(updateFlag, Currency_Mosaic_Id, levy, signer);
				
				test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), levy, (validSigner)? signer:owner);
				cache.commit(Height());
				
				AssertValidationResult(expectedResult, notification, cache);
			}
		}

		// region validator tests

		TEST(TEST_CLASS, MosaicIdNotFound) {

			auto cache = test::LevyCacheFactory::Create();

			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = model::MosaicLevy();
			auto notification = model::MosaicUpdateLevyNotification<1>(0, Currency_Mosaic_Id, levy, signer);
			
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, notification,  cache);
		}

		TEST(TEST_CLASS, IneligibleSigner) {
			AssertLevyParameters(Failure_Mosaic_Ineligible_Signer, CreateTestMosaicLevy(Currency_Mosaic_Id), 0, false);
		}

		TEST(TEST_CLASS, InvalidRecipient) {
			AssertLevyParameters(Failure_Mosaic_Recipient_Levy_Not_Exist, CreateTestMosaicLevy(Currency_Mosaic_Id),
				model::MosaicLevyModifyBitChangeRecipient, true);
		}

		TEST(TEST_CLASS, InvalidLevyFee) {
			AssertLevyParameters(Failure_Mosaic_Invalid_Levy_Fee, CreateTestMosaicLevy(Currency_Mosaic_Id, Amount(0)),
				model::MosaicLevyModifyBitChangeLevyFee, true);
		}
		
		TEST(TEST_CLASS, MosaicLevyIdNotFound) {
			AssertLevyParameters(Failure_Mosaic_Id_Not_Found, CreateTestMosaicLevy(MosaicId(664)),
				model::MosaicLevyModifyBitChangeMosaicId, true);
		}

		TEST(TEST_CLASS, UpdateLevyOK) {
			AssertLevyParameters(ValidationResult::Success, CreateTestMosaicLevy(Currency_Mosaic_Id),
				model::MosaicLevyModifyBitChangeMosaicId, true);
		}
		// endregion
	}
}
