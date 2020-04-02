#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/MosaicTestUtils.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "catapult/constants.h"

namespace catapult {
	namespace validators {

#define TEST_CLASS MosaicLevyDefinitionValidatorTests
		
		DEFINE_COMMON_VALIDATOR_TESTS(MosaicLevyDefinition,)
		
		namespace {
			constexpr UnresolvedMosaicId unresMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(unresMosaicId.unwrap());
			
			model::MosaicDefinitionNotification<1> CreateDefaultNotification(const Key& signer, const model::MosaicLevy& levy) {
				auto properties = model::MosaicProperties::FromValues({ { 3, 6, 15 } });
				return model::MosaicDefinitionNotification<1>(signer, Currency_Mosaic_Id, properties, levy);
			}
			
			model::MosaicLevy CreateDummyLevy() {
				auto levy = test::CreateMosaicLevy();
				levy.Type = model::LevyType::Absolute;
				return levy;
			}
			
			/// This itself looks like a common function, all Notification inherit from Notification object
			void AssertValidationResult(ValidationResult expectedResult, model::MosaicDefinitionNotification<1> notification,
			                            cache::CatapultCache &cache) {
				// Arrange:
				auto pValidator = CreateMosaicLevyDefinitionValidator();
				
				auto result = test::ValidateNotification(*pValidator, notification, cache, test::CreateMosaicResolverContextDefault());
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
			
			catapult::Address ResolveAddress(UnresolvedAddress address) {
				auto resolverContext = test::CreateMosaicResolverContextDefault();
				return resolverContext.resolve(address);
			}
		}
		
		// region validator tests
		
		TEST(TEST_CLASS, MosaicLevyIsNotAvailable) {
			auto cache = test::MosaicCacheFactory::Create();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = CreateDefaultNotification(signer, test::CreateMosaicLevy());
			AssertValidationResult(ValidationResult::Success, notification, cache);
		}
		
		TEST(TEST_CLASS, RecipientAddressIsInvalid) {
			auto cache = test::MosaicCacheFactory::Create();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto notification = CreateDefaultNotification(signer, CreateDummyLevy());
			AssertValidationResult(Failure_Mosaic_Recipient_Levy_Not_Exist, notification, cache);
		}
		
		TEST(TEST_CLASS, MosaicLevyIdNotFound) {
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			
			auto signer = test::GenerateRandomByteArray<Key>();
			
			auto levy = test::CreateCorrectMosaicLevy();
			auto sinkResolved = ResolveAddress(levy.Recipient);
			
			test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, Amount(100));
			test::SetCacheBalances(delta, sinkResolved, test::CreateMosaicBalance(levy.MosaicId, Amount(1)));
			cache.commit(Height());
			
			auto notification = CreateDefaultNotification(signer, levy);
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, notification, cache);
		}
		
		TEST(TEST_CLASS, LevyFeeIsInvalid) {
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = CreateDummyLevy();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto sinkResolved = ResolveAddress(recipient);
			levy.Recipient = recipient;
			
			test::SetCacheBalances(delta, sinkResolved, test::CreateMosaicBalance(Currency_Mosaic_Id, Amount(1)));
			cache.commit(Height());
			
			auto notification = CreateDefaultNotification(signer, levy);
			AssertValidationResult(Failure_Mosaic_Invalid_Levy_Fee, notification, cache);
		}
		
		TEST(TEST_CLASS, LevyFeeIsInvalidPercentile) {
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();
			auto levy = model::MosaicLevy();
			auto recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
			
			auto sinkResolved = ResolveAddress(recipient);
			
			levy.MosaicId = MosaicId(0);
			levy.Recipient = recipient;
			levy.Fee = test::CreateMosaicLevyFeePercentile(300);    // 300%
			levy.Type = model::LevyType::Percentile;
			
			test::SetCacheBalances(delta, sinkResolved, test::CreateMosaicBalance(Currency_Mosaic_Id, Amount(1)));
			cache.commit(Height());
			
			auto notification = CreateDefaultNotification(signer, levy);
			AssertValidationResult(Failure_Mosaic_Invalid_Levy_Fee, notification, cache);
		}
		
		TEST(TEST_CLASS, MosaicDefineWithLevyOK) {
			auto cache = test::MosaicCacheFactory::Create();
			auto delta = cache.createDelta();
			
			auto signer = test::GenerateRandomByteArray<Key>();
			
			auto levy = test::CreateCorrectMosaicLevy();
	
			auto sinkResolved = ResolveAddress(levy.Recipient);
	
			test::AddMosaic(delta, levy.MosaicId, Height(1), Eternal_Artifact_Duration, Amount(100));
			test::SetCacheBalances(delta, sinkResolved, test::CreateMosaicBalance(levy.MosaicId, Amount(1)));
			cache.commit(Height());
			
			auto notification = CreateDefaultNotification(signer, levy);
			AssertValidationResult(ValidationResult::Success, notification, cache);
		}
		
		// endregion
	}
}
