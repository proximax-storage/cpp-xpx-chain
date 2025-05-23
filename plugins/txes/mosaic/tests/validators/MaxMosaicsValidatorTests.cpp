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
#include "src/config/MosaicConfiguration.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

	DEFINE_COMMON_VALIDATOR_TESTS(MaxMosaicsBalanceTransfer)
	DEFINE_COMMON_VALIDATOR_TESTS(MaxMosaicsSupplyChange)

#define BALANCE_TRANSFER_TEST_CLASS BalanceTransferMaxMosaicsValidatorTests
#define SUPPLY_CHANGE_TEST_CLASS SupplyChangeMaxMosaicsValidatorTests

	namespace {
		auto CreateCache() {
			return test::CoreSystemCacheFactory::Create();
		}

		template<typename TKey>
		auto CreateAndSeedCache(const TKey& key) {
			auto cache = test::CoreSystemCacheFactory::Create();
			{
				auto cacheDelta = cache.createDelta();
				auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(key, Height());
				auto& accountState = accountStateCacheDelta.find(key).get();
				for (auto i = 0u; i < 5; ++i)
					accountState.Balances.credit(MosaicId(i + 1), Amount(1));

				cache.commit(Height());
			}

			return cache;
		}

		auto CreateConfig(uint16_t maxMosaics) {
			auto pluginConfig = config::MosaicConfiguration::Uninitialized();
			pluginConfig.MaxMosaicsPerAccount = maxMosaics;
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.SetPluginConfiguration(pluginConfig);
			return networkConfig;
		}

		void RunBalanceTransferTest(ValidationResult expectedResult, uint16_t maxMosaics, UnresolvedMosaicId mosaicId, Amount amount, bool seedCache = true) {
			// Arrange:
			auto owner = test::GenerateRandomByteArray<Key>();
			auto recipient = test::GenerateRandomByteArray<Address>();
			auto unresolvedRecipient = test::UnresolveXor(recipient);
			auto cache = seedCache ? CreateAndSeedCache(recipient) : CreateCache();

			auto config = CreateConfig(maxMosaics);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pValidator = CreateMaxMosaicsBalanceTransferValidator();
			auto notification = model::BalanceTransferNotification<1>(owner, unresolvedRecipient, mosaicId, amount);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pConfigHolder->Config());

			// Assert:
			EXPECT_EQ(expectedResult, result) << "maxMosaics " << maxMosaics << ", mosaicId " << mosaicId << ", amount " << amount;
		}
	}

	TEST(BALANCE_TRANSFER_TEST_CLASS, FailureWhenMaximumIsExceeded) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		RunBalanceTransferTest(Failure_Mosaic_Max_Mosaics_Exceeded, 1, test::UnresolveXor(MosaicId(6)), Amount(100));
		RunBalanceTransferTest(Failure_Mosaic_Max_Mosaics_Exceeded, 4, test::UnresolveXor(MosaicId(6)), Amount(100));
		RunBalanceTransferTest(Failure_Mosaic_Max_Mosaics_Exceeded, 5, test::UnresolveXor(MosaicId(6)), Amount(100));
	}

	TEST(BALANCE_TRANSFER_TEST_CLASS, SuccessWhenRecipientAccountDoesntExist) {
		// Act:
		RunBalanceTransferTest(ValidationResult::Success, 5, test::UnresolveXor(MosaicId(6)), Amount(100), false);
	}

	TEST(BALANCE_TRANSFER_TEST_CLASS, SuccessWhenAmountIsZero) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		RunBalanceTransferTest(ValidationResult::Success, 5, test::UnresolveXor(MosaicId(6)), Amount(0));
	}

	TEST(BALANCE_TRANSFER_TEST_CLASS, SuccessWhenAccountAlreadyOwnsThatMosaic) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		RunBalanceTransferTest(ValidationResult::Success, 5, test::UnresolveXor(MosaicId(3)), Amount(100));
	}

	TEST(BALANCE_TRANSFER_TEST_CLASS, SuccessWhenMaximumIsNotExceeded) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		RunBalanceTransferTest(ValidationResult::Success, 6, test::UnresolveXor(MosaicId(6)), Amount(100));
		RunBalanceTransferTest(ValidationResult::Success, 10, test::UnresolveXor(MosaicId(6)), Amount(100));
		RunBalanceTransferTest(ValidationResult::Success, 123, test::UnresolveXor(MosaicId(6)), Amount(100));
	}

	namespace {
		void RunMosaicSupplyTest(
				ValidationResult expectedResult,
				uint16_t maxMosaics,
				UnresolvedMosaicId mosaicId,
				model::MosaicSupplyChangeDirection direction) {
			// Arrange:
			auto owner = test::GenerateRandomByteArray<Key>();
			auto cache = CreateAndSeedCache(owner);

			auto config = CreateConfig(maxMosaics);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pValidator = CreateMaxMosaicsSupplyChangeValidator();
			auto notification = model::MosaicSupplyChangeNotification<1>(owner, mosaicId, direction, Amount(100));

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pConfigHolder->Config());

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "maxMosaics " << maxMosaics
					<< ", mosaicId " << mosaicId
					<< ", direction " << utils::to_underlying_type(direction);
		}
	}

	TEST(SUPPLY_CHANGE_TEST_CLASS, FailureWhenMaximumIsExceeded) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		auto direction = model::MosaicSupplyChangeDirection::Increase;
		RunMosaicSupplyTest(Failure_Mosaic_Max_Mosaics_Exceeded, 1, test::UnresolveXor(MosaicId(6)), direction);
		RunMosaicSupplyTest(Failure_Mosaic_Max_Mosaics_Exceeded, 4, test::UnresolveXor(MosaicId(6)), direction);
		RunMosaicSupplyTest(Failure_Mosaic_Max_Mosaics_Exceeded, 5, test::UnresolveXor(MosaicId(6)), direction);
	}

	TEST(SUPPLY_CHANGE_TEST_CLASS, SuccessWhenSupplyIsDecreased) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		auto direction = model::MosaicSupplyChangeDirection::Decrease;
		RunMosaicSupplyTest(ValidationResult::Success, 5, test::UnresolveXor(MosaicId(6)), direction);
	}

	TEST(SUPPLY_CHANGE_TEST_CLASS, SuccessWhenAccountAlreadyOwnsThatMosaic) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		auto direction = model::MosaicSupplyChangeDirection::Increase;
		RunMosaicSupplyTest(ValidationResult::Success, 5, test::UnresolveXor(MosaicId(3)), direction);
	}

	TEST(SUPPLY_CHANGE_TEST_CLASS, SuccessWhenMaximumIsNotExceeded) {
		// Act: account in test already owns 5 mosaics with ids 1 to 5
		auto direction = model::MosaicSupplyChangeDirection::Increase;
		RunMosaicSupplyTest(ValidationResult::Success, 6, test::UnresolveXor(MosaicId(6)), direction);
		RunMosaicSupplyTest(ValidationResult::Success, 10, test::UnresolveXor(MosaicId(6)), direction);
		RunMosaicSupplyTest(ValidationResult::Success, 123, test::UnresolveXor(MosaicId(6)), direction);
	}
}}
