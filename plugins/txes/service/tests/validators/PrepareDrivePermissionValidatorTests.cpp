/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS PrepareDrivePermissionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(PrepareDrivePermission, )

	namespace {
		using Notification = model::PrepareDriveNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry* pDriveEntry,
				const Key& driveKey) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			if (!!pDriveEntry) {
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(*pDriveEntry);
				cache.commit(currentHeight);
			}
			Notification notification(driveKey, Key(), BlockDuration(), BlockDuration(), Amount(), 0, 0, 0, 0);
			auto pValidator = CreatePrepareDrivePermissionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenDriveExists) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Already_Exists,
			&driveEntry,
			driveEntry.key());
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			nullptr,
			test::GenerateRandomByteArray<Key>());
	}
}}
