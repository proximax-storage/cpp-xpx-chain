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

#define TEST_CLASS ExchangeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Exchange, )

	namespace {
		using Notification = model::ExchangeNotification<1>;

		constexpr MosaicId Storage_Mosaic_Id(1234);
		const Key Long_Offer_Key = test::GenerateRandomByteArray<Key>();

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
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
				const state::ExchangeEntry exchangeEntry,
				const std::vector<model::MatchedOffer>& offers = {},
				const Amount& driveBalance = Amount(0)) {
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
				driveAccount.Balances.credit(Storage_Mosaic_Id, driveBalance);
				accountStateCacheDelta.addAccount(driveAccount);

				auto& exchangeCacheDelta = delta.sub<cache::ExchangeCache>();
				exchangeCacheDelta.insert(exchangeEntry);

				cache.commit(currentHeight);
			}
			Notification notification(signer, offers.size(), offers.data());
			auto pValidator = CreateExchangeValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenNotDriveAccount) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			test::GenerateRandomByteArray<Key>(),
			exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenDriveNotInPendingState) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Not_In_Pending_State,
			driveEntry,
			driveEntry.key(),
			exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenDriveProcessedFullDuration) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		driveEntry.billingHistory().push_back(state::BillingPeriodDescription{ Height(101), Height(1),
			{ { test::GenerateRandomByteArray<Key>(), Amount(10), Height(100) } } });
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Processed_Full_Duration,
			driveEntry,
			driveEntry.key(),
			exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenNoDefaultExchangeEntry) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
			driveEntry,
			driveEntry.key(),
			exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenNoDefaultExchangeOffer) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
			driveEntry,
			driveEntry.key(),
			exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenDefaultExchangeOfferInsufficient) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(0), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
			driveEntry,
			driveEntry.key(),
			exchangeEntry);
	}

	TEST(TEST_CLASS, FailureWhenExchangeMosaicIdInvalid) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			Failure_Service_Exchange_Of_This_Mosaic_Is_Not_Allowed,
			driveEntry,
			driveEntry.key(),
			exchangeEntry,
			{ { { { test::UnresolveXor(MosaicId(4321)), Amount(10) }, Amount(10), model::OfferType::Sell }, Long_Offer_Key } });
	}

	TEST(TEST_CLASS, FailureWhenMatchedOfferTypeInvalid) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			Failure_Service_Exchange_Of_This_Mosaic_Is_Not_Allowed,
			driveEntry,
			driveEntry.key(),
			exchangeEntry,
			{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(10) }, Amount(10), model::OfferType::Buy }, Long_Offer_Key } });
	}

	TEST(TEST_CLASS, FailureWhenMatchedOfferAmountInvalid) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			Failure_Service_Exchange_More_Than_Required,
			driveEntry,
			driveEntry.key(),
			exchangeEntry,
			{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(100) }, Amount(10), model::OfferType::Sell }, Long_Offer_Key } });
	}

	TEST(TEST_CLASS, FailureWhenMatchedOfferCostInvalid) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			Failure_Service_Exchange_Cost_Is_Worse_Than_Default,
			driveEntry,
			driveEntry.key(),
			exchangeEntry,
			{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(10) }, Amount(100), model::OfferType::Sell }, Long_Offer_Key } });
	}

	TEST(TEST_CLASS, SuccessWhenMatchedOfferValid) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		driveEntry.setState(state::DriveState::Pending);
		driveEntry.setDuration(BlockDuration(100));
		state::ExchangeEntry exchangeEntry(Long_Offer_Key);
		exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
			state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			driveEntry.key(),
			exchangeEntry,
			{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(10) }, Amount(10), model::OfferType::Sell }, Long_Offer_Key } });
	}
}}
