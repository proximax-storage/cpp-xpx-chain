/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/LevyTestUtils.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicModifyLevyValidatorTests
		
		DEFINE_COMMON_VALIDATOR_TESTS(ModifyLevy,)
		
		/// region helper functions
		namespace {
			auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
			using LevySetupFunc = std::function<void (cache::CatapultCacheDelta&, const Key&)>;
			
			model::MosaicModifyLevyNotification<1> CreateDefaultNotification(const Key& signer, const model::MosaicLevyRaw& levy) {
				auto xORLevy = model::MosaicLevyRaw(levy.Type,
					levy.Recipient,
					test::UnresolveXor(MosaicId(levy.MosaicId.unwrap())),
					levy.Fee);
				
				return model::MosaicModifyLevyNotification<1>(test::UnresolveXor(Currency_Mosaic_Id), xORLevy, signer);
			}
			
			model::MosaicLevyRaw CreateValidMosaicLevyRaw()
			{
				auto levy = model::MosaicLevyRaw();
				levy.Type = model::LevyType::Absolute;
				levy.Recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
				levy.MosaicId = UnresolvedMosaicId(1000);
				levy.Fee = test::CreateMosaicLevyFeePercentile(10); // 10% levy fee
				return levy;
			}
			
			model::MosaicLevyRaw CreateLevyWithPercentile(float percentage) {
				auto levy = CreateValidMosaicLevyRaw();
				levy.Type = model::LevyType::Percentile;
				levy.Fee = test::CreateMosaicLevyFeePercentile(percentage);    // 300%

				return levy;
			}
			
			model::MosaicLevyRaw CreateLevyWithType() {
				auto levy = CreateValidMosaicLevyRaw();
				levy.Type = model::LevyType::Absolute;
				levy.Fee = Amount(0);
				levy.Recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
				return levy;
			}
			
			model::MosaicLevyRaw CreateValidMosaicLevyXor(const UnresolvedMosaicId& mosaicId)
			{
				auto recipient = test::GenerateRandomByteArray<Address>();
				auto levy = model::MosaicLevyRaw();
				levy.Type = model::LevyType::Absolute;
				levy.Recipient = test::UnresolveXor(recipient);
				levy.MosaicId = mosaicId;
				levy.Fee = test::CreateMosaicLevyFeePercentile(10); // 10% levy fee
				return levy;
			}
			
			catapult::Address ResolveAddress(UnresolvedAddress address) {
				auto resolverContext = test::CreateResolverContextXor();
				return resolverContext.resolve(address);
			}
			
			test::BalanceTransfers CreateMosaicBalance(MosaicId id, Amount amount) {
				return {{id, amount}};
			}
			
			/// This itself looks like a common function, all Notification inherit from Notification object
			void AssertValidationResult(ValidationResult expectedResult,
				model::MosaicModifyLevyNotification<1> notification,
			    cache::CatapultCache &cache) {
				
				// Arrange:
				auto pValidator = CreateModifyLevyValidator();
				
				auto result = test::ValidateNotification(*pValidator, notification, cache);
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
			
			void SetupBaseMosaic(cache::CatapultCacheDelta& delta, const Key& signer)
			{
				/// add the base mosaic where we will add the levy
				test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Amount(100), signer);
			}
			
			void AssertLevyParameterTest(ValidationResult expectedResult,  const model::MosaicLevyRaw& levy,
			                             bool validRecipient, const MosaicId& recipientBalanceMosaicId,
			                             bool validSigner, LevySetupFunc action) {
				
				auto cache = test::LevyCacheFactory::Create();
				auto delta = cache.createDelta();
				auto owner = test::GenerateRandomByteArray<Key>();
				auto signer = test::GenerateRandomByteArray<Key>();
				
				auto notification = CreateDefaultNotification(signer, levy);
				
				action(delta, (validSigner)? signer:owner);
				
				if( validRecipient ) {
					auto sinkResolved = ResolveAddress(levy.Recipient);
					test::SetCacheBalances(delta, sinkResolved,
						CreateMosaicBalance(recipientBalanceMosaicId, Amount(1)));
				}
				
				cache.commit(Height());
				
				AssertValidationResult(expectedResult, notification, cache);
			}
		}
		/// end region
		
		// region validator tests
		
		TEST(TEST_CLASS, BaseMosaicIdNotFound) {
			AssertLevyParameterTest(Failure_Mosaic_Id_Not_Found,
				CreateValidMosaicLevyRaw(), false,
			    MosaicId(0), false, [](cache::CatapultCacheDelta&, const Key&) {});
		}
		
		TEST(TEST_CLASS, SignerIsInvalid) {
			AssertLevyParameterTest(Failure_Mosaic_Ineligible_Signer,
				CreateValidMosaicLevyRaw(), false,
				MosaicId(0), false, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, RecipientAddressIsInvalid) {
			AssertLevyParameterTest(Failure_Mosaic_Recipient_Levy_Not_Exist,
				CreateLevyWithType(), false,
				MosaicId(0), true, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, MosaicLevyIdNotFound) {
			AssertLevyParameterTest(Failure_Mosaic_Id_Not_Found,
				CreateValidMosaicLevyXor(UnresolvedMosaicId(1000)), true,
				MosaicId(1000), true, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, LevyFeeIsInvalid) {
			AssertLevyParameterTest(Failure_Mosaic_Invalid_Levy_Fee,
				CreateLevyWithType(), true, Currency_Mosaic_Id, true, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, LevyFeeIsInvalidPercentile) {
			AssertLevyParameterTest(Failure_Mosaic_Invalid_Levy_Fee,
				CreateLevyWithPercentile(300), true,
				Currency_Mosaic_Id, true, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, AddMosaicLevyOK) {
			AssertLevyParameterTest(ValidationResult::Success,
				CreateValidMosaicLevyXor(Unresolved_Mosaic_Id),
				true, Currency_Mosaic_Id, true, SetupBaseMosaic);
		}

		// endregion
	}
}
