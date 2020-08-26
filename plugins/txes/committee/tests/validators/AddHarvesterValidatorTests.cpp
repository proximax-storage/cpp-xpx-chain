/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/validators/Validators.h"
#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS AddHarvesterValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AddHarvester, )

	namespace {
		using Notification = model::AddHarvesterNotification<1>;

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
				const state::CommitteeEntry& entry,
				const Key& harvesterKey,
				const Amount& harvesterBalance = Min_Harvester_Balance) {
			// Arrange:
			Height currentHeight(1);
			auto config = CreateConfig();
			auto cache = test::CommitteeCacheFactory::Create(config);
			{
				auto delta = cache.createDelta();
				auto& committeeCache = delta.sub<cache::CommitteeCache>();
				committeeCache.insert(entry);

				auto& accountCache = delta.sub<cache::AccountStateCache>();
				accountCache.addAccount(harvesterKey, currentHeight);
				auto& accountState = accountCache.find(harvesterKey).get();
				accountState.Balances.track(Harvesting_Mosaic_Id);
				accountState.Balances.credit(Harvesting_Mosaic_Id, harvesterBalance, currentHeight);

				cache.commit(currentHeight);
			}
			Notification notification(harvesterKey);
			auto pValidator = CreateAddHarvesterValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenHarvesterRegistered) {
		// Arrange:
		auto entry = test::CreateCommitteeEntry();

		// Assert:
		AssertValidationResult(
			Failure_Committee_Redundant,
			entry,
			entry.key());
	}

	TEST(TEST_CLASS, FailureWhenHarvesterIneligible) {
		// Arrange:
		auto entry = test::CreateCommitteeEntry();

		// Assert:
		AssertValidationResult(
			Failure_Committee_Harvester_Ineligible,
			entry,
			test::GenerateRandomByteArray<Key>(),
			Amount(10));
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		auto entry = test::CreateCommitteeEntry();

		AssertValidationResult(
			ValidationResult::Success,
			entry,
			test::GenerateRandomByteArray<Key>());
	}
}}
