/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/validators/Validators.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS ModifyAddressMetadataValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ModifyAddressMetadata,)

	namespace {
		const Key Account_Public_Key = test::GenerateRandomByteArray<Key>();
		const Address Account_Address = model::PublicKeyToAddress(Account_Public_Key, model::NetworkIdentifier::Zero);

		void PopulateCache(cache::CatapultCache& cache) {
			auto delta = cache.createDelta();
			auto& accountCacheDelta = delta.sub<cache::AccountStateCache>();
			accountCacheDelta.addAccount(Account_Public_Key, Height(1));

			cache.commit(Height(1));
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const UnresolvedAddress& metadataId,
				Key signer) {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::MetadataConfiguration::Uninitialized();
			config.Network.SetPluginConfiguration(pluginConfig);
			auto cache = test::MetadataCacheFactory::Create(config.ToConst());
			PopulateCache(cache);
			auto pValidator = CreateModifyAddressMetadataValidator();
			auto notification = model::ModifyAddressMetadataNotification_v1(signer, metadataId);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenMetadataIdEqualsToSignerAndAccountExist) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			extensions::CopyToUnresolvedAddress(Account_Address),
			Account_Public_Key);
	}

	TEST(TEST_CLASS, FailueWhenMetadataIdEqualsToSignerButAccountNotExist) {
		// Arrange:
		auto publicKey = test::GenerateRandomByteArray<Key>();
		auto accountAddress	= model::PublicKeyToAddress(publicKey, model::NetworkIdentifier::Zero);

		// Act:
		AssertValidationResult(
				Failure_Metadata_Address_Not_Found,
				extensions::CopyToUnresolvedAddress(accountAddress),
				publicKey);
	}

	TEST(TEST_CLASS, FailueWhenMetadataIdNotEqualsToSignerButAccountNotExist) {
		// Act:
		AssertValidationResult(
				Failure_Metadata_Address_Modification_Not_Permitted,
				extensions::CopyToUnresolvedAddress(Account_Address),
				test::GenerateRandomByteArray<Key>());
	}
}}
