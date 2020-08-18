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

	DEFINE_COMMON_VALIDATOR_TESTS(PrepareDrivePermissionV1, )
	DEFINE_COMMON_VALIDATOR_TESTS(PrepareDrivePermissionV2, )

	namespace {

		template<typename TTraits>
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
			model::PrepareDriveNotification<TTraits::Version> notification(driveKey, Key(), BlockDuration(), BlockDuration(), Amount(), 0, 0, 0, 0);
			auto pValidator = TTraits::CreateValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	struct PreparePermissionValidatorV1Traits {
		static constexpr VersionType Version = 1;
		static auto CreateValidator() {
			return CreatePrepareDrivePermissionV1Validator();
		}
	};

	struct PreparePermissionValidatorV2Traits {
		static constexpr VersionType Version = 2;
		static auto CreateValidator() {
			return CreatePrepareDrivePermissionV2Validator();
		}
	};

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PreparePermissionValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PreparePermissionValidatorV2Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(FailureWhenDriveExists) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult<TTraits>(
			Failure_Service_Drive_Already_Exists,
			&driveEntry,
			driveEntry.key());
	}

	TRAITS_BASED_TEST(Success) {
		// Assert:
		AssertValidationResult<TTraits>(
			ValidationResult::Success,
			nullptr,
			test::GenerateRandomByteArray<Key>());
	}
}}
