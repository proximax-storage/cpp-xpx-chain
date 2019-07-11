/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/validators/Validators.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ModifyMosaicMetadataValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ModifyMosaicMetadata,)

	namespace {
		const Key Mosaic_Owner = test::GenerateRandomData<Key_Size>();
		constexpr MosaicId Mosaic_Id = MosaicId(3);

		void PopulateCache(cache::CatapultCache& cache) {
			auto delta = cache.createDelta();
			auto& mosaicCacheDelta = delta.sub<cache::MosaicCache>();
			
			model::MosaicProperties::PropertyValuesContainer values{};
			auto definition = state::MosaicDefinition(Height(1), Mosaic_Owner, 3, model::MosaicProperties::FromValues(values));
			auto entry = state::MosaicEntry(Mosaic_Id, definition);
			entry.increaseSupply(Amount(666));
			mosaicCacheDelta.insert(entry);

			cache.commit(Height(1));
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const UnresolvedMosaicId& metadataId,
				Key signer) {
			// Arrange:
			auto config = model::BlockChainConfiguration::Uninitialized();
			auto pluginConfig = config::MetadataConfiguration::Uninitialized();
			const_cast<model::BlockChainConfiguration&>(config).SetPluginConfiguration(PLUGIN_NAME(metadata), pluginConfig);
			auto cache = test::MetadataCacheFactory::Create(config);
			PopulateCache(cache);
			auto pValidator = CreateModifyMosaicMetadataValidator();
			auto notification = model::ModifyMosaicMetadataNotification_v1(signer, metadataId);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenMosaicExistAndOwnerValid) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			UnresolvedMosaicId(Mosaic_Id.unwrap()),
			Mosaic_Owner);
	}

	TEST(TEST_CLASS, FailureWhenMosaicDoesNotExistAndOwnerValid) {
		// Act:
		AssertValidationResult(
				Failure_Metadata_Mosaic_Is_Not_Exist,
				UnresolvedMosaicId(4),
				Mosaic_Owner);
	}

	TEST(TEST_CLASS, FailureWhenMosaicExistsButOwnerNotValid) {
		// Act:
		AssertValidationResult(
				Failure_Metadata_Mosaic_Modification_Not_Permitted,
				UnresolvedMosaicId(Mosaic_Id.unwrap()),
				test::GenerateRandomData<Key_Size>());
	}

	TEST(TEST_CLASS, FailureWhenMosaicDoesNotExistButOwnerNotValid) {
		// Act:
		AssertValidationResult(
				Failure_Metadata_Mosaic_Is_Not_Exist,
				UnresolvedMosaicId(4),
				test::GenerateRandomData<Key_Size>());
	}
}}
