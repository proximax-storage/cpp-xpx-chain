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
#include "tests/test/LevyTestUtils.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "catapult/constants.h"

namespace catapult {
	namespace validators {

#define TEST_CLASS MosaicAddLevyValidatorTests
		
		DEFINE_COMMON_VALIDATOR_TESTS(AddLevy,)
		
		/// region helper functions
		namespace {
			auto Currency_Mosaic_Id = MosaicId(1234);
			using LevySetupFunc = std::function<void (cache::CatapultCacheDelta&, const Key&)>;
			
			model::MosaicAddLevyNotification<1> CreateDefaultNotification(const Key& signer, const model::MosaicLevy& levy) {
				return model::MosaicAddLevyNotification<1>(Currency_Mosaic_Id, levy, signer);
			}
			
			model::MosaicLevy CreateLevyWithType() {
				auto levy = model::MosaicLevy();
				levy.Type = model::LevyType::Absolute;
				return levy;
			}
			
			model::MosaicLevy CreateLevyWithPercentile(float percentage) {
				auto levy = model::MosaicLevy();
				levy.Type = model::LevyType::Percentile;
				levy.Fee = test::CreateMosaicLevyFeePercentile(percentage);    // 300%

				return levy;
			}
			
			model::MosaicLevy CreateValidMosaicLevyXor(const MosaicId& mosaicId)
			{
				auto recipient = test::GenerateRandomByteArray<Address>();
				auto levy = model::MosaicLevy();
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
			void AssertValidationResult(ValidationResult expectedResult, model::MosaicAddLevyNotification<1> notification,
			                            cache::CatapultCache &cache) {
				
				// Arrange:
				auto pValidator = CreateAddLevyValidator();
				
				auto result = test::ValidateNotification(*pValidator, notification, cache);
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
			
			void SetupBaseMosaic(cache::CatapultCacheDelta& delta, const Key& signer)
			{
				/// add the base mosaic where we will add the levy
				test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Amount(100), signer);
			}
			
			void SetupBaseMosaicWithLevy(cache::CatapultCacheDelta& delta, const Key& signer)
			{
				test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), test::CreateValidMosaicLevy(), signer);
			}
			
			void AssertLevyParameterTest(ValidationResult expectedResult,  const model::MosaicLevy& levy,
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
				test::CreateValidMosaicLevy(), false,
			    MosaicId(0), false, [](cache::CatapultCacheDelta&, const Key&) {});
		}
		
		TEST(TEST_CLASS, LevyAlreadyExist) {
			AssertLevyParameterTest(Failure_Mosaic_Levy_Already_Exist,
				CreateValidMosaicLevyXor(Currency_Mosaic_Id), true,
				Currency_Mosaic_Id, true, SetupBaseMosaicWithLevy);
		}
		
		TEST(TEST_CLASS, SignerIsInvalid) {
			AssertLevyParameterTest(Failure_Mosaic_Ineligible_Signer,
				test::CreateValidMosaicLevy(), false,
				MosaicId(0), false, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, RecipientAddressIsInvalid) {
			AssertLevyParameterTest(Failure_Mosaic_Recipient_Levy_Not_Exist,
				CreateLevyWithType(), false,
				MosaicId(0), true, SetupBaseMosaic);
		}
		
		TEST(TEST_CLASS, MosaicLevyIdNotFound) {
			AssertLevyParameterTest(Failure_Mosaic_Id_Not_Found,
				CreateValidMosaicLevyXor(MosaicId(1000)), true,
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
				CreateValidMosaicLevyXor(MosaicId(0)),
				true, Currency_Mosaic_Id, true, SetupBaseMosaic);
		}

		// endregion
	}
}
