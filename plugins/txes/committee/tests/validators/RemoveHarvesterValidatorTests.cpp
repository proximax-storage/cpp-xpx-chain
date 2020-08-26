/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CommitteeCache.h"
#include "src/validators/Validators.h"
#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS RemoveHarvesterValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(RemoveHarvester, )

	namespace {
		using Notification = model::RemoveHarvesterNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::CommitteeEntry& entry,
				const Key& harvesterKey) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::CommitteeCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::CommitteeCache>();
				driveCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(harvesterKey);
			auto pValidator = CreateRemoveHarvesterValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenHarvesterNotRegistered) {
		// Arrange:
		auto entry = test::CreateCommitteeEntry();

		// Assert:
		AssertValidationResult(
			Failure_Committee_Account_Does_Not_Exist,
			entry,
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		auto entry = test::CreateCommitteeEntry();

		AssertValidationResult(
			ValidationResult::Success,
			entry,
			entry.key());
	}
}}
