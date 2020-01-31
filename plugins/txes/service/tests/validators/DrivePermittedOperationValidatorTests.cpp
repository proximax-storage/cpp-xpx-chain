/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/exchange/src/model/ExchangeTransaction.h"
#include "src/cache/DriveCache.h"
#include "src/validators/Validators.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DrivePermittedOperationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DrivePermittedOperation, )

	namespace {
		using Notification = model::AggregateEmbeddedTransactionNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry* pEntry,
				const Key& signer,
				const model::UniqueEntityPtr<mocks::EmbeddedMockTransaction>& pTransaction) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			if (pEntry) {
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(*pEntry);
				cache.commit(currentHeight);
			}
			Notification notification(signer, *pTransaction, 0, nullptr);
			auto pValidator = CreateDrivePermittedOperationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccesWhenDriveDoesntExist) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		auto pTransaction = mocks::CreateEmbeddedMockTransaction(0);
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_PrepareDrive,
			model::Entity_Type_JoinToDrive,
			model::Entity_Type_DriveFileSystem,
			model::Entity_Type_FilesDeposit,
			model::Entity_Type_EndDrive,
			model::Entity_Type_DriveFilesReward,
			model::Entity_Type_Start_Drive_Verification,
			model::Entity_Type_End_Drive_Verification,
			model::Entity_Type_Exchange_Offer,
			model::Entity_Type_Exchange,
			model::Entity_Type_Remove_Exchange_Offer,
		});

		// Assert:
		for (const auto& type : types) {
			pTransaction->Type = type;
			AssertValidationResult(
				ValidationResult::Success,
				nullptr,
				signer,
				pTransaction);
		}
	}

	TEST(TEST_CLASS, FailureWhenOperationNotPermitted) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		auto pTransaction = mocks::CreateEmbeddedMockTransaction(0);
		pTransaction->Signer = entry.key();
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_PrepareDrive,
			model::Entity_Type_JoinToDrive,
			model::Entity_Type_DriveFileSystem,
			model::Entity_Type_FilesDeposit,
			model::Entity_Type_Start_Drive_Verification,
			model::Entity_Type_Exchange_Offer,
			model::Entity_Type_Remove_Exchange_Offer,
		});

		// Assert:
		for (const auto& type : types) {
			pTransaction->Type = type;
			AssertValidationResult(
				Failure_Service_Operation_Is_Not_Permitted,
				&entry,
				signer,
				pTransaction);
		}
	}

	TEST(TEST_CLASS, SuccessWhenOperationPermitted) {
		// Arrange:
		auto signer = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
		auto pTransaction = mocks::CreateEmbeddedMockTransaction(0);
		pTransaction->Signer = entry.key();
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_EndDrive,
			model::Entity_Type_DriveFilesReward,
			model::Entity_Type_End_Drive_Verification,
			model::Entity_Type_Exchange,
		});

		// Assert:
		for (const auto& type : types) {
			pTransaction->Type = type;
			AssertValidationResult(
				ValidationResult::Success,
				&entry,
				signer,
				pTransaction);
		}
	}
}}
