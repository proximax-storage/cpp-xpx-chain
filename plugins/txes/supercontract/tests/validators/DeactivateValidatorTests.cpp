/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS DeactivateValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Deactivate, )

	namespace {
		using Notification = model::DeactivateNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::SuperContractEntry& entry,
				const Key& signer,
				const Key& driveKey) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& superContractCacheDelta = delta.sub<cache::SuperContractCache>();
				superContractCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(signer, entry.key(), driveKey);
			auto pValidator = CreateDeactivateValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setOwner(test::GenerateRandomByteArray<Key>());
			entry.setMainDriveKey(test::GenerateRandomByteArray<Key>());

			return entry;
		}

		struct OwnerSignerTraits {
			static Key signer(state::SuperContractEntry& entry) {
				return entry.owner();
			}
		};

		struct DriveSignerTraits {
			static Key signer(state::SuperContractEntry& entry) {
				return entry.mainDriveKey();
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_OwnerSigner) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<OwnerSignerTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_DriveSigner) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<DriveSignerTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TEST(TEST_CLASS, FailureWhenSignerInvalid) {
		// Arrange:
		auto entry = CreateSuperContractEntry();

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			entry,
			test::GenerateRandomByteArray<Key>(),
			entry.mainDriveKey());
	}

	TRAITS_BASED_TEST(FailureWhenDriveKeyInvalid) {
		// Arrange:
		auto entry = CreateSuperContractEntry();

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Invalid_Drive_Key,
			entry,
			TTraits::signer(entry),
			test::GenerateRandomByteArray<Key>());
	}

	TRAITS_BASED_TEST(FailureWhenExecutionInProgress) {
		// Arrange:
		auto entry = CreateSuperContractEntry();
		entry.setExecutionCount(10);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Execution_Is_In_Progress,
			entry,
			TTraits::signer(entry),
			entry.mainDriveKey());
	}

	TRAITS_BASED_TEST(Success) {
		// Arrange:
		auto entry = CreateSuperContractEntry();

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			TTraits::signer(entry),
			entry.mainDriveKey());
	}
}}
