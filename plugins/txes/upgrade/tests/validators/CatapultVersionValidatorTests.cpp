/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/version/version.h"
#include "src/cache/BlockchainUpgradeCache.h"
#include "src/cache/BlockchainUpgradeCacheStorage.h"
#include "src/validators/Validators.h"
#include "tests/test/BlockchainUpgradeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include <limits>

namespace catapult { namespace validators {

#define TEST_CLASS BlockchainVersionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(BlockchainVersion,)

	namespace {
		void AssertValidationResult(
			ValidationResult expectedResult,
			uint64_t nextBlockchainVersion,
			bool seedCache) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache<test::BlockchainUpgradeCacheFactory>();
			if (seedCache) {
				auto delta = cache.createDelta();
				auto& upgradeCacheDelta = delta.sub<cache::BlockchainUpgradeCache>();
				upgradeCacheDelta.insert(state::BlockchainUpgradeEntry(Height(1), BlockchainVersion{nextBlockchainVersion}));
				cache.commit(Height(1));
			}
			model::BlockNotification<1> notification(Key(), Key(), Timestamp(), Difficulty(), 0, 0);
			auto pValidator = CreateBlockchainVersionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config::BlockchainConfiguration::Uninitialized(), Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenCurrentBlockchainVersionTooLow) {
		// Assert:
		AssertValidationResult(
			Failure_BlockchainUpgrade_Invalid_Current_Version,
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

	TEST(TEST_CLASS, SuccessWhenCurrentBlockchainVersionIsValid) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			0,
			true);
	}
}}
