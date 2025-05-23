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

#define TEST_CLASS EndDriveValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndDrive, )

	namespace {
		using Notification = model::EndDriveNotification<1>;

		constexpr MosaicId Currency_Mosaic_Id(1234);
		constexpr MosaicId Storage_Mosaic_Id(4321);
		const Key Long_Offer_Key = test::GenerateRandomByteArray<Key>();

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			auto pluginConfig = config::ExchangeConfiguration::Uninitialized();
			pluginConfig.LongOfferKey = Long_Offer_Key;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				state::DriveEntry& driveEntry,
				const Key& signer,
				const state::ExchangeEntry* pExchangeEntry = nullptr) {
			// Arrange:
			Height currentHeight(1);
			driveEntry.setBillingPrice(Amount(10));
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();

				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);

				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				auto driveAccount = test::CreateAccount(driveEntry.key());
				accountStateCacheDelta.addAccount(driveAccount);

				if (!!pExchangeEntry) {
					auto& exchangeCacheDelta = delta.sub<cache::ExchangeCache>();
					exchangeCacheDelta.insert(*pExchangeEntry);
				}

				cache.commit(currentHeight);
			}
			Notification notification(driveEntry.key(), signer);
			auto pValidator = CreateEndDriveValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenOwnerSigned) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			driveEntry.owner());
	}

	TEST(TEST_CLASS, FailureWhenDriveNotInPendingState) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Not_In_Pending_State,
			driveEntry,
			driveEntry.key());
	}

	TEST(TEST_CLASS, SuccessWhenDurationPassed) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.billingHistory().push_back(state::BillingPeriodDescription{ Height(101), Height(1),
			{ { test::GenerateRandomByteArray<Key>(), Amount(10), Height(100) } } });

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			driveEntry.key());
	}

	TEST(TEST_CLASS, FailureWhenNoDefaultExchangeEntry) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
			driveEntry,
			driveEntry.key());
	}

	TEST(TEST_CLASS, FailureWhenNoDefaultExchangeOffer) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
			driveEntry,
			driveEntry.key(),
			&exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenDefaultExchangeOfferInsufficient) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(0), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
			driveEntry,
			driveEntry.key(),
			&exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenDriveCurrencyBalanceInsufficient) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			driveEntry.key(),
			&exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenInvalidSigner) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Operation_Is_Not_Permitted,
			driveEntry,
			test::GenerateRandomByteArray<Key>());
	}
}}
