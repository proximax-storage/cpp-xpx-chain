/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/AddressTestUtils.h"
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
			                            cache::CatapultCache &cache,
			                            const config::BlockchainConfiguration& config = config::BlockchainConfiguration::Uninitialized()) {
				
				// Arrange:
				auto pValidator = CreateRemoveLevyValidator();
				auto notification = model::MosaicRemoveLevyNotification<1>(test::UnresolveXor(mosaicId), signer);
				
				auto result = test::ValidateNotification(*pValidator, notification, cache, config);
				
				// Assert:
				EXPECT_EQ(expectedResult, result);
			}
			
			config::BlockchainConfiguration CreateMosaicConfigWithLevy(const Key& key) {
				test::MutableBlockchainConfiguration config;
				auto pluginConfig = config::MosaicConfiguration::Uninitialized();
				config.Network.Info.PublicKey = key;
				config.Network.SetPluginConfiguration(pluginConfig);
				return config.ToConst();
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
		
		TEST(TEST_CLASS, NemesisSigner) {
			auto signer = test::GenerateRandomByteArray<Key>();
			auto config = CreateMosaicConfigWithLevy(signer);
			auto cache = test::LevyCacheFactory::Create(config);
			auto delta = cache.createDelta();
			
			auto owner = test::GenerateRandomByteArray<Key>();
			auto levy = test::CreateValidMosaicLevy();
			
			/// add the base mosaic where we will add the levy
			test::AddMosaicWithLevy(delta, Currency_Mosaic_Id, Height(1), levy, owner);
			cache.commit(Height());
			
			AssertValidationResult(ValidationResult::Success, Currency_Mosaic_Id, signer, cache, config);
		}
		
		TEST(TEST_CLASS, NemesisSignToRemoveExpiredBaseMosaic) {
			auto signer = test::GenerateRandomByteArray<Key>();
			auto config = CreateMosaicConfigWithLevy(signer);
			auto cache = test::LevyCacheFactory::Create(config);
			auto delta = cache.createDelta();
			
			/// create levy entry without base mosaic to simulate expired base mosaic
			auto& levyCacheDelta = delta.sub<cache::LevyCache>();
			auto levyData = test::CreateValidMosaicLevy();
			auto levyEntry = test::CreateLevyEntry(Currency_Mosaic_Id, levyData, true, false );
			levyCacheDelta.insert(levyEntry);
			cache.commit(Height());
			
			AssertValidationResult(ValidationResult::Success, Currency_Mosaic_Id, signer, cache, config);
		}
		
		TEST(TEST_CLASS, MosaicLevyNotFound) {
			auto cache = test::LevyCacheFactory::Create();
			auto delta = cache.createDelta();
			auto signer = test::GenerateRandomByteArray<Key>();

			test::AddMosaic(delta, Currency_Mosaic_Id, Height(1), Eternal_Artifact_Duration, signer);
			cache.commit(Height());
			
			AssertValidationResult(Failure_Mosaic_Levy_Entry_Not_Found, Currency_Mosaic_Id, signer, cache);
		}
		
		TEST(TEST_CLASS, MosaicIdNotFound) {
			auto cache = test::LevyCacheFactory::Create();
			auto signer = test::GenerateRandomByteArray<Key>();
			AssertValidationResult(Failure_Mosaic_Id_Not_Found, Currency_Mosaic_Id, signer, cache);
		}
		
		// endregion
	}
}
