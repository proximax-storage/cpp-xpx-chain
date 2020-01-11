/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ExchangeCache.h"
#include "src/config/ExchangeConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS OfferValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Offer, )

	namespace {
		using Notification = model::OfferNotification<1>;

		constexpr MosaicId Currency_Mosaic_Id(1234);

		auto CreateConfig(const Key& longOfferKey) {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			auto pluginConfig = config::ExchangeConfiguration::Uninitialized();
			pluginConfig.MaxOfferDuration = BlockDuration{500};
			pluginConfig.LongOfferKey = longOfferKey;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::vector<model::OfferWithDuration> offers = {},
				const Key& signer = test::GenerateRandomByteArray<Key>(),
				const state::ExchangeEntry* pEntry = nullptr,
				const Key& longOfferKey = test::GenerateRandomByteArray<Key>()) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::ExchangeCacheFactory::Create();
			if (pEntry) {
				auto delta = cache.createDelta();
				auto& exchangeDelta = delta.sub<cache::ExchangeCache>();
				exchangeDelta.insert(*pEntry);
				cache.commit(currentHeight);
			}

			Notification notification(signer, offers.size(), offers.data());
			auto config = CreateConfig(longOfferKey);
			auto pValidator = CreateOfferValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		struct SellOfferTraits {
			static constexpr auto OfferType = model::OfferType::Sell;
			static void InsertOffer(state::ExchangeEntry& entry) {
				entry.sellOffers().emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(1)}});
			}
		};

		struct BuyOfferTraits {
			static constexpr auto OfferType = model::OfferType::Buy;
			static void InsertOffer(state::ExchangeEntry& entry) {
				entry.buyOffers().emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(1)}, Amount(100)});
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Sell) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SellOfferTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Buy) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BuyOfferTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TEST(TEST_CLASS, FailureWhenNoOffers) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_No_Offers);
	}

	TEST(TEST_CLASS, FailureWhenDuplicatedOffersInRequest) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_Duplicated_Offer_In_Request,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(100)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(5)}, Amount(20), model::OfferType::Buy}, BlockDuration(100)},
			});
	}

	TEST(TEST_CLASS, FailureWhenOfferDurationZero) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_Zero_Offer_Duration,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(0)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferDurationTooLarge) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_Offer_Duration_Too_Large,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TTraits::OfferType}, BlockDuration(1000)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferZeroAmount) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_Zero_Amount,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(0)}, Amount(100), TTraits::OfferType}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferZeroPrice) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_Zero_Price,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(0), TTraits::OfferType}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenSellOfferMosaicNotAllowed) {
		// Assert:
		AssertValidationResult(
			Failure_Exchange_Mosaic_Not_Allowed,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(Currency_Mosaic_Id), Amount(10)}, Amount(100), TTraits::OfferType}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferAlreadyExists) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry entry(offerOwner);
		TTraits::InsertOffer(entry);

		// Assert:
		AssertValidationResult(
			Failure_Exchange_Offer_Exists,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TTraits::OfferType}, BlockDuration(100)},
			},
			offerOwner,
			&entry);
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(100)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(200)},
			},
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, Success_LongOffer) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(1000)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(2000)},
			},
			offerOwner,
			nullptr,
			offerOwner);
	}
}}
