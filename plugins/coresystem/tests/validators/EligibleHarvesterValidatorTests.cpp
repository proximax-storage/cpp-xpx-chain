/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/validators/Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Block.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/validators/ValidatorContext.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS EligibleHarvesterValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EligibleHarvester, config::CreateMockConfigurationHolder())

	namespace {
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);
		constexpr auto Importance_Grouping = 234u;

		auto CreateEmptyCatapultCache(const config::BlockchainConfiguration& config) {
			const_cast<config::ImmutableConfiguration&>(config.Immutable).HarvestingMosaicId = Harvesting_Mosaic_Id;
			const_cast<model::NetworkConfiguration&>(config.Network).ImportanceGrouping = Importance_Grouping;
			return test::CreateEmptyCatapultCache(config);
		}

		void AddAccount(
				cache::CatapultCache& cache,
				const Key& publicKey,
				Amount balance) {
			auto delta = cache.createDelta();
			auto& accountCache = delta.sub<cache::AccountStateCache>();
			accountCache.addAccount(publicKey, Height(100));
			accountCache.find(publicKey).get().Balances.credit(Harvesting_Mosaic_Id, balance, Height(100));
			cache.commit(Height());
		}
	}

	TEST(TEST_CLASS, FailureIfAccountIsUnknown) {
		// Arrange:
		test::MutableBlockchainConfiguration mutableConfig;
		mutableConfig.Network.MinHarvesterBalance = Amount(1234);
		auto config = mutableConfig.ToConst();
		auto cache = CreateEmptyCatapultCache(config);
		auto key = test::GenerateRandomByteArray<Key>();
		auto height = Height(1000);
		AddAccount(cache, key, Amount(9999));
		auto pConfigHolder = config::CreateMockConfigurationHolder(config);

		auto pValidator = CreateEligibleHarvesterValidator(pConfigHolder);

		auto signer = test::GenerateRandomByteArray<Key>();
		auto notification = test::CreateBlockNotification(signer);

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, height);

		// Assert:
		EXPECT_EQ(Failure_Core_Block_Harvester_Ineligible, result);
	}

	namespace {
		void AssertValidationResult(
				ValidationResult expectedResult,
				int64_t minBalanceDelta,
				Height blockHeight) {
			// Arrange:
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.MinHarvesterBalance = Amount(1234);
			auto config = mutableConfig.ToConst();
			auto cache = CreateEmptyCatapultCache(config);
			auto key = test::GenerateRandomByteArray<Key>();
			auto initialBalance = Amount(static_cast<Amount::ValueType>(1234 + minBalanceDelta));
			AddAccount(cache, key, initialBalance);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);

			auto pValidator = CreateEligibleHarvesterValidator(pConfigHolder);
			auto notification = test::CreateBlockNotification(key);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, blockHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureIfBalanceIsBelowMinBalance) {
		// Assert:
		constexpr auto expectedResult = Failure_Core_Block_Harvester_Ineligible;
		auto height = Height(10000);
		AssertValidationResult(expectedResult, -1, height);
		AssertValidationResult(expectedResult, -100, height);
	}

	TEST(TEST_CLASS, SuccessIfAllCriteriaAreMet) {
		// Assert:
		constexpr auto expectedResult = ValidationResult::Success;
		auto height = Height(10000);
		AssertValidationResult(expectedResult, 0, height);
		AssertValidationResult(expectedResult, 1, height);
		AssertValidationResult(expectedResult, 12345, height);
	}
}}
