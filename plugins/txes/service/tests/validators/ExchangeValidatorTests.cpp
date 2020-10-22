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

		DEFINE_COMMON_VALIDATOR_TESTS(ExchangeV1,)
		DEFINE_COMMON_VALIDATOR_TESTS(ExchangeV2,)

		namespace {
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

			template<typename TTraits>
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


				model::ExchangeNotification<TTraits::Version> notification(signer, offers.size(), offers.data());
				auto pValidator = TTraits::CreateValidator();

				// Act:
				auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(), currentHeight);

				// Assert:
				EXPECT_EQ(expectedResult, result);
			}

			struct ValidatorV1Traits {
				static constexpr VersionType Version = 1;
				static auto CreateValidator() {
					return CreateExchangeV1Validator();
				}
			};

			struct ValidatorV2Traits {
				static constexpr VersionType Version = 2;
				static auto CreateValidator() {
					return CreateExchangeV2Validator();
				}
			};
		}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ValidatorV2Traits>(); } \
	template<typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

		TRAITS_BASED_TEST(SuccessWhenNotDriveAccount) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					ValidationResult::Success,
					driveEntry,
					test::GenerateRandomByteArray<Key>(),
					exchangeEntry);
		}

		TRAITS_BASED_TEST(FailureWhenDriveNotInPendingState) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Drive_Not_In_Pending_State,
					driveEntry,
					driveEntry.key(),
					exchangeEntry);
		}

		TRAITS_BASED_TEST(FailureWhenDriveProcessedFullDuration) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			driveEntry.billingHistory().push_back(state::BillingPeriodDescription{ Height(101), Height(1),
																				   { { test::GenerateRandomByteArray<Key>(), Amount(10), Height(100) } } });
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Drive_Processed_Full_Duration,
					driveEntry,
					driveEntry.key(),
					exchangeEntry);
		}

		TRAITS_BASED_TEST(FailureWhenNoDefaultExchangeEntry) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(test::GenerateRandomByteArray<Key>());

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
					driveEntry,
					driveEntry.key(),
					exchangeEntry);
		}

		TRAITS_BASED_TEST(FailureWhenNoDefaultExchangeOffer) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
					driveEntry,
					driveEntry.key(),
					exchangeEntry);
		}

		TRAITS_BASED_TEST(FailureWhenDefaultExchangeOfferInsufficient) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);
			exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
											   state::SellOffer{ { Amount(0), Amount(10), Amount(10), Height(-1) } });

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Drive_Cant_Find_Default_Exchange_Offer,
					driveEntry,
					driveEntry.key(),
					exchangeEntry);
		}

		TRAITS_BASED_TEST(FailureWhenExchangeMosaicIdInvalid) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);
			exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
											   state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Exchange_Of_This_Mosaic_Is_Not_Allowed,
					driveEntry,
					driveEntry.key(),
					exchangeEntry,
					{ { { { test::UnresolveXor(MosaicId(4321)), Amount(10) }, Amount(10), model::OfferType::Sell }, Long_Offer_Key } });
		}

		TRAITS_BASED_TEST(FailureWhenMatchedOfferTypeInvalid) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);
			exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
											   state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Exchange_Of_This_Mosaic_Is_Not_Allowed,
					driveEntry,
					driveEntry.key(),
					exchangeEntry,
					{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(10) }, Amount(10), model::OfferType::Buy }, Long_Offer_Key } });
		}

		TRAITS_BASED_TEST(FailureWhenMatchedOfferAmountInvalid) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);
			exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
											   state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Exchange_More_Than_Required,
					driveEntry,
					driveEntry.key(),
					exchangeEntry,
					{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(100) }, Amount(10), model::OfferType::Sell }, Long_Offer_Key } });
		}

		TRAITS_BASED_TEST(FailureWhenMatchedOfferCostInvalid) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);
			exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
											   state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Service_Exchange_Cost_Is_Worse_Than_Default,
					driveEntry,
					driveEntry.key(),
					exchangeEntry,
					{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(10) }, Amount(100), model::OfferType::Sell }, Long_Offer_Key } });
		}

		TRAITS_BASED_TEST(SuccessWhenMatchedOfferValid) {
			// Arrange:
			state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
			driveEntry.setState(state::DriveState::Pending);
			driveEntry.setDuration(BlockDuration(100));
			state::ExchangeEntry exchangeEntry(Long_Offer_Key);
			exchangeEntry.sellOffers().emplace(Storage_Mosaic_Id,
											   state::SellOffer{ { Amount(10), Amount(10), Amount(10), Height(-1) } });

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					ValidationResult::Success,
					driveEntry,
					driveEntry.key(),
					exchangeEntry,
					{ { { { test::UnresolveXor(Storage_Mosaic_Id), Amount(10) }, Amount(10), model::OfferType::Sell }, Long_Offer_Key } });
		}
	}}
