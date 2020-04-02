#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "catapult/constants.h"
#include "src/model/MosaicModifyLevyTransaction.h"
#include "tests/test/MosaicTestUtils.h"

namespace catapult {
	namespace validators {

#define TEST_CLASS MosaicModifyLevyValidatorTests

		DEFINE_COMMON_VALIDATOR_TESTS(ModifyMosaicLevy,)

		namespace {
			constexpr UnresolvedMosaicId unresMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(unresMosaicId.unwrap());
			
			void AssertValidationResult(ValidationResult expectedResult, model::MosaicModifyLevyNotification<1> notification,
			                       cache::CatapultCache &cache) {
				// Arrange:
				auto pValidator = CreateModifyMosaicLevyValidator();

				// Act:
				auto result = test::ValidateNotification(*pValidator, notification, cache, test::CreateMosaicResolverContextDefault());
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
			
			model::MosaicLevy CreateDummyMosaicLevy() {
				auto levy = test::CreateMosaicLevy();
				levy.Type = model::LevyType::Absolute;
				levy.MosaicId = Currency_Mosaic_Id;
				levy.Fee = Amount(10);
				return levy;
			}
		}

		// region validator tests

		TEST(TEST_CLASS, MosaicIdNotFound) {

			auto cache = test::MosaicCacheFactory::Create();

			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = test::CreateMosaicLevy();
			auto notification = model::MosaicModifyLevyNotification<1>(0, Currency_Mosaic_Id, levy, signer);
			
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, notification,  cache);
		}

		TEST(TEST_CLASS, IneligibleSigner) {

			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();

			auto signer = test::GenerateRandomByteArray<Key>();
			auto editor = test::GenerateRandomByteArray<Key>();
			auto levy = CreateDummyMosaicLevy();
			auto notification = model::MosaicModifyLevyNotification<1>(0, Currency_Mosaic_Id, test::CreateMosaicLevy(), editor);
			
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, levy, signer);
			cache.commit(Height());

			AssertValidationResult(Failure_Mosaic_Ineligible_Signer, notification, cache);
		}

		TEST(TEST_CLASS, InvalidRecipient) {

			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();

			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = CreateDummyMosaicLevy();
			
			auto notification = model::MosaicModifyLevyNotification<1>(
				model::MosaicLevyModifyBitChangeRecipient, Currency_Mosaic_Id, test::CreateMosaicLevy(), signer);
			
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, levy, signer);
			cache.commit(Height());

			AssertValidationResult(Failure_Mosaic_Recipient_Levy_Not_Exist, notification, cache);
		}

		TEST(TEST_CLASS, InvalidLevyFee) {

			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();

			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = CreateDummyMosaicLevy();
			
			auto notification = model::MosaicModifyLevyNotification<1>(model::MosaicLevyModifyBitChangeLevyFee,
				Currency_Mosaic_Id, test::CreateMosaicLevy(), signer);
			
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration,levy, signer);
			cache.commit(Height());

			AssertValidationResult(Failure_Mosaic_Invalid_Levy_Fee, notification, cache);
		}
		
		TEST(TEST_CLASS, MosaicLevyIdNotFound) {
			
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			
			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = CreateDummyMosaicLevy();
			
			levy.MosaicId = MosaicId(664);
			auto notification = model::MosaicModifyLevyNotification<1>(model::MosaicLevyModifyBitChangeMosaicId,
				Currency_Mosaic_Id, levy, signer);
			
			test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, signer);
			cache.commit(Height());
			
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, notification, cache);
		}

		TEST(TEST_CLASS, ModifyLevyOK) {

			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();

			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = CreateDummyMosaicLevy();
			auto modify = test::CreateMosaicLevy();
			
			auto notification = model::MosaicModifyLevyNotification<1>(
				model::MosaicLevyModifyBitChangeLevyFee | model::MosaicLevyModifyBitChangeType, Currency_Mosaic_Id, modify, signer);
			
			modify.Type = model::LevyType::Percentile;
			modify.Fee = Amount(100);
			
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration,levy, signer);
			cache.commit(Height());

			AssertValidationResult(ValidationResult::Success, notification, cache);
		}
		// endregion
	}
}
