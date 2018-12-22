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
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/validators/ValidatorContext.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS EligibleHarvesterValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EligibleHarvester, Amount(1234))

	namespace {
		constexpr auto Importance_Grouping = 234u;

		auto CreateEmptyCatapultCache() {
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.ImportanceGrouping = Importance_Grouping;
			return test::CreateEmptyCatapultCache(config);
		}

		void AddAccount(
				cache::CatapultCache& cache,
				const Key& publicKey,
				Amount balance) {
			auto delta = cache.createDelta();
			auto& accountCache = delta.sub<cache::AccountStateCache>();
			accountCache.addAccount(publicKey, Height(100));
			accountCache.find(publicKey).get().Balances.credit(Xpx_Id, balance, Height(100));
			cache.commit(Height());
		}
	}

	TEST(TEST_CLASS, FailureIfAccountIsUnknown) {
		// Arrange:
		auto cache = CreateEmptyCatapultCache();
		auto key = test::GenerateRandomData<Key_Size>();
		auto height = Height(1000);
		AddAccount(cache, key, Amount(9999));

		auto pValidator = CreateEligibleHarvesterValidator(Amount(1234));

		auto signer = test::GenerateRandomData<Key_Size>();
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
			auto cache = CreateEmptyCatapultCache();
			auto key = test::GenerateRandomData<Key_Size>();
			auto initialBalance = Amount(static_cast<Amount::ValueType>(1234 + minBalanceDelta));
			AddAccount(cache, key, initialBalance);

			auto pValidator = CreateEligibleHarvesterValidator(Amount(1234));
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
