/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MaxFilesOnDriveValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MaxFilesOnDrive, )

	namespace {
		using Notification = model::DriveFileSystemNotification<1>;

		constexpr uint16_t Max_Files_On_Drive = 10;
		constexpr uint64_t Drive_Size = 1000;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::ServiceConfiguration::Uninitialized();
			pluginConfig.MaxFilesOnDrive = Max_Files_On_Drive;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& driveEntry,
				const std::vector<model::AddAction>& addActions,
				const std::vector<model::RemoveAction>& removeActions) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);
				cache.commit(currentHeight);
			}

			Notification notification(driveEntry.key(), Key(), Hash256(), Hash256(),
				addActions.size(), addActions.data(), removeActions.size(), removeActions.data());
			auto pValidator = CreateMaxFilesOnDriveValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					CreateConfig(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenMaximumExceeded) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		for (auto i = 0u; i < Max_Files_On_Drive; ++i)
			driveEntry.files().emplace(test::GenerateRandomByteArray<Hash256>(), state::FileInfo{ test::Random() });

		// Assert:
		AssertValidationResult(
			Failure_Service_Too_Many_Files_On_Drive,
			driveEntry,
			{ model::AddAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() } },
			{});
	}

	TEST(TEST_CLASS, FailureWhenOccupiedSizeOverflows) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setSize(Drive_Size);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Size_Exceeded,
			driveEntry,
			{
				model::AddAction{ { test::GenerateRandomByteArray<Hash256>() }, std::numeric_limits<uint64_t>::max() }
			},
			{});
	}

	TEST(TEST_CLASS, FailureWhenDriveSizeExceeded) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setSize(Drive_Size);
		driveEntry.setOccupiedSpace(Drive_Size);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Size_Exceeded,
			driveEntry,
			{
				model::AddAction{ { test::GenerateRandomByteArray<Hash256>() }, Drive_Size },
				model::AddAction{ { test::GenerateRandomByteArray<Hash256>() }, Drive_Size }
			},
			{ model::RemoveAction{ { { test::GenerateRandomByteArray<Hash256>() }, Drive_Size } } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setSize(Drive_Size);
		driveEntry.setOccupiedSpace(Drive_Size);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			{
				model::AddAction{ { test::GenerateRandomByteArray<Hash256>() }, Drive_Size / 2 },
				model::AddAction{ { test::GenerateRandomByteArray<Hash256>() }, Drive_Size / 2 }
			},
			{ model::RemoveAction{ { { test::GenerateRandomByteArray<Hash256>() }, Drive_Size } } });
	}
}}
