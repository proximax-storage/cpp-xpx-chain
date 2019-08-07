/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CatapultUpgradeCache.h"
#include "src/cache/CatapultUpgradeCacheStorage.h"
#include "src/config/CatapultUpgradeConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/CatapultUpgradeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

namespace catapult { namespace validators {

#define TEST_CLASS CatapultVersionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(CatapultVersion,)

	namespace {
		void AssertValidationResult(
			ValidationResult expectedResult,
			uint64_t nextCatapultVersion,
			bool seedCache) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache<test::CatapultUpgradeCacheFactory>(model::BlockChainConfiguration::Uninitialized());
			if (seedCache) {
				auto delta = cache.createDelta();
				auto& upgradeCacheDelta = delta.sub<cache::CatapultUpgradeCache>();
				upgradeCacheDelta.insert(state::CatapultUpgradeEntry(Height(1), CatapultVersion{nextCatapultVersion}));
				cache.commit(Height(1));
			}
			model::BlockNotification<1> notification(Key(), Key(), Timestamp(), Difficulty(), 0, 0);
			auto pValidator = CreateCatapultVersionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenCurrentCatapultVersionTooLow) {
		// Assert:
		AssertValidationResult(
			Failure_CatapultUpgrade_Invalid_Current_Catapult_Version,
			std::numeric_limits<uint64_t>::max(),
			true);
	}

	TEST(TEST_CLASS, SuccessWhenNoUpgrade) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			std::numeric_limits<uint64_t>::max(),
			false);
	}

	TEST(TEST_CLASS, SuccessWhenCurrentCatapultVersionIsValid) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			0,
			true);
	}
}}
