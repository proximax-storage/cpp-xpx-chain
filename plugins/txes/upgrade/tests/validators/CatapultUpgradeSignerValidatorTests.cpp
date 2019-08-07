/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS CatapultUpgradeSignerValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(CatapultUpgradeSigner,)

	namespace {
		void AssertValidationResult(const Key& signer, const Key& networkKey, const ValidationResult& expectedResult) {
			// Arrange:
			auto notification = model::CatapultUpgradeSignerNotification<1>(signer);
			auto cache = test::CreateEmptyCatapultCache(model::BlockChainConfiguration::Uninitialized());
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto context = test::CreateValidatorContext(Height(), model::NetworkInfo(model::NetworkIdentifier::Mijin_Test, networkKey, GenerationHash()), readOnlyCache);
			auto pValidator = CreateCatapultUpgradeSignerValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenSignerAndNetworkPublicKeyMatch) {
		// Arrange:
		auto key = test::GenerateKeyPair().publicKey();
		// Assert:
		AssertValidationResult(key, key, ValidationResult::Success);
	}

	TEST(TEST_CLASS, FailureWhenSignerAndNetworkPublicKeyDontMatch) {
		// Assert:
		AssertValidationResult(test::GenerateKeyPair().publicKey(), test::GenerateKeyPair().publicKey(), Failure_CatapultUpgrade_Invalid_Signer);
	}
}}
