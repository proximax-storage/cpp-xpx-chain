/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCache.h"
#include "src/config/ServiceConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MaxFilesOnDriveValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MaxFilesOnDrive, config::CreateMockConfigurationHolder())

	namespace {
		using Notification = model::DriveFileSystemNotification<1>;

		constexpr uint16_t Max_Files_On_Drive = 10;
		constexpr uint16_t Add_Actions_Count = 5;

		auto CreateConfigHolder() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::ServiceConfiguration::Uninitialized();
			pluginConfig.MaxFilesOnDrive = Max_Files_On_Drive;
			config.Network.SetPluginConfiguration(PLUGIN_NAME_HASH(service), pluginConfig);
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& driveEntry) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);
				cache.commit(currentHeight);
			}
			Notification notification(driveEntry.key(), Key(), Hash256(), Hash256(), Add_Actions_Count, nullptr, 0, nullptr);
			auto pValidator = CreateMaxFilesOnDriveValidator(CreateConfigHolder());

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenMaximumExceeded) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		for (auto i = 0u; i < Max_Files_On_Drive - Add_Actions_Count + 2; ++i)
			driveEntry.files().emplace(test::GenerateRandomByteArray<Hash256>(), state::FileInfo{ test::Random() });

		// Assert:
		AssertValidationResult(
			Failure_Service_Too_Many_Files_On_Drive,
			driveEntry);
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			state::DriveEntry(test::GenerateRandomByteArray<Key>()));
	}
}}
