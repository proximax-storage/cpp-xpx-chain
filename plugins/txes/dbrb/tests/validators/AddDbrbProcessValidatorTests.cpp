/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/validators/Validators.h"
#include "tests/test/DbrbTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS AddDbrbProcessValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AddDbrbProcess, )

	namespace {
		using Notification = model::AddDbrbProcessNotification<1>;

		constexpr MosaicId Harvesting_Mosaic_Id(1234);
		constexpr Amount Min_Harvester_Balance(100);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.Network.MinHarvesterBalance = Min_Harvester_Balance;
			return config.ToConst();
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DbrbProcessEntry& entry,
				const Key& harvesterKey,
				const Amount& harvesterBalance = Min_Harvester_Balance) {
			// Arrange:
			Height currentHeight(1);
			auto config = CreateConfig();
			auto cache = test::DbrbProcessCacheFactory::Create(config);
			{
				auto delta = cache.createDelta();
				auto& dbrbProcessCache = delta.sub<cache::DbrbViewCache>();
				dbrbProcessCache.insert(entry);

				auto& accountCache = delta.sub<cache::AccountStateCache>();
				accountCache.addAccount(harvesterKey, currentHeight);
				auto& accountState = accountCache.find(harvesterKey).get();
				accountState.Balances.track(Harvesting_Mosaic_Id);
				accountState.Balances.credit(Harvesting_Mosaic_Id, harvesterBalance, currentHeight);

				cache.commit(currentHeight);
			}
			Notification notification(entry.processId());
			auto pValidator = CreateAddDbrbProcessValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenDbrbProcessRegistered) {
		// Arrange:
		auto entry = test::CreateDbrbProcessEntry();

		// Assert:
		AssertValidationResult(
				Failure_Dbrb_Process_Not_Expired,
				entry,
				entry.processId());
	}

	TEST(TEST_CLASS, FailureWhenDbrbProcessIneligible) {
		// Arrange:
		auto entry = test::CreateDbrbProcessEntry();

		// Assert:
		AssertValidationResult(
				Failure_Dbrb_Process_Not_Expired,
				entry,
				test::GenerateRandomByteArray<Key>(),
				Amount(10));
	}

	TEST(TEST_CLASS, Failure_Dbrb_Process_Not_Expired) {
		// Assert:
		auto entry = test::CreateDbrbProcessEntry();

		AssertValidationResult(
				Failure_Dbrb_Process_Not_Expired,
				entry,
				test::GenerateRandomByteArray<Key>());
	}
}}
