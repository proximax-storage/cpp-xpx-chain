/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <plugins/txes/mosaic/tests/test/MosaicCacheTestUtils.h>
#include "src/cache/ExchangeCache.h"
#include "src/config/ExchangeConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS OfferValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(OfferV1, )
	DEFINE_COMMON_VALIDATOR_TESTS(OfferV2, )

	namespace {
		constexpr MosaicId Currency_Mosaic_Id(1234);
		using MosaicSetup = std::function<void(cache::CatapultCacheDelta&)>;
		
		auto CreateConfig(const Key& longOfferKey) {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			auto pluginConfig = config::ExchangeConfiguration::Uninitialized();
			pluginConfig.MaxOfferDuration = BlockDuration{500};
			pluginConfig.LongOfferKey = longOfferKey;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		model::MosaicProperties CreateMosaicPropertiesWithDuration(BlockDuration duration) {
			model::MosaicProperties::PropertyValuesContainer values{};
			values[utils::to_underlying_type(model::MosaicPropertyId::Duration)] = duration.unwrap();
			return model::MosaicProperties::FromValues(values);
		}
		
		void AddMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height, BlockDuration duration, Amount supply) {
			auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
			auto definition = state::MosaicDefinition(height, Key(), 1, CreateMosaicPropertiesWithDuration(duration));
			auto entry = state::MosaicEntry(id, definition);
			entry.increaseSupply(supply);
			mosaicCacheDelta.insert(entry);
		}
		
		void AddMosaicEternal(cache::CatapultCacheDelta& cache, MosaicId id, Amount supply) {
			auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
			auto definition = state::MosaicDefinition(Height(1), Key(), 1,  model::MosaicProperties::FromValues({}));
			auto entry = state::MosaicEntry(id, definition);
			entry.increaseSupply(supply);
			mosaicCacheDelta.insert(entry);
		}
		
		template<typename TTraits>
		void AssertValidationResultBase(
				ValidationResult expectedResult,
				MosaicSetup setup,
				const std::vector<model::OfferWithDuration> offers = {},
				const Key& signer = test::GenerateRandomByteArray<Key>(),
				const state::ExchangeEntry* pEntry = nullptr,
				const Key& longOfferKey = test::GenerateRandomByteArray<Key>()) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::ExchangeCacheFactory::Create();
			
			auto delta = cache.createDelta();
			setup(delta);
			cache.commit(Height());
			
			if (pEntry) {
				auto& exchangeDelta = delta.sub<cache::ExchangeCache>();
				exchangeDelta.insert(*pEntry);
				cache.commit(currentHeight);
			}

			model::OfferNotification<TTraits::Version> notification(signer, offers.size(), offers.data());
			auto config = CreateConfig(longOfferKey);
			auto pValidator = TTraits::CreateValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
			
		template<typename TTraits>
		void AssertValidationResult(
			ValidationResult expectedResult,
			const std::vector<model::OfferWithDuration> offers = {},
			const Key& signer = test::GenerateRandomByteArray<Key>(),
			const state::ExchangeEntry* pEntry = nullptr,
			const Key& longOfferKey = test::GenerateRandomByteArray<Key>()) {
			
			AssertValidationResultBase<TTraits>(
				expectedResult,
				[](cache::CatapultCacheDelta& delta){
					AddMosaicEternal(delta, MosaicId(1), Amount(100));
					AddMosaicEternal(delta, MosaicId(2), Amount(100));
					AddMosaicEternal(delta, Currency_Mosaic_Id, Amount(100));
				}, offers, signer, pEntry, longOfferKey);
		}
		
		template<typename TTraits>
		void AssertValidationResultExpiringMosaic(
			ValidationResult expectedResult,
			const std::vector<model::OfferWithDuration> offers = {},
			const Key& signer = test::GenerateRandomByteArray<Key>(),
			const state::ExchangeEntry* pEntry = nullptr,
			const Key& longOfferKey = test::GenerateRandomByteArray<Key>()) {
			
			AssertValidationResultBase<TTraits>(
				expectedResult,
				[](cache::CatapultCacheDelta& delta){
					AddMosaic(delta, MosaicId(1), Height(1), BlockDuration(50), Amount(100));
					AddMosaic(delta, MosaicId(2), Height(1), BlockDuration(50),Amount(100));
					AddMosaic(delta, Currency_Mosaic_Id, Height(1), BlockDuration(50),Amount(100));
				}, offers, signer, pEntry, longOfferKey);
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

		struct ValidatorV1Traits {
			static constexpr VersionType Version = 1;
			static auto CreateValidator() {
				return CreateOfferV1Validator();
			}
		};

		struct ValidatorV2Traits {
			static constexpr VersionType Version = 2;
			static auto CreateValidator() {
				return CreateOfferV2Validator();
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TOfferTraits, typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Sell_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SellOfferTraits, ValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Buy_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BuyOfferTraits, ValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Sell_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SellOfferTraits, ValidatorV2Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Buy_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BuyOfferTraits, ValidatorV2Traits>(); } \
	template<typename TOfferTraits, typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(FailureWhenNoOffers) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_No_Offers);
	}

	TRAITS_BASED_TEST(FailureWhenDuplicatedOffersInRequest) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Duplicated_Offer_In_Request,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(100)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(5)}, Amount(20), model::OfferType::Buy}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferDurationZero) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Zero_Offer_Duration,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(0)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferDurationTooLarge) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Offer_Duration_Too_Large,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, BlockDuration(1000)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferZeroAmount) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Zero_Amount,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(0)}, Amount(100), TOfferTraits::OfferType}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferZeroPrice) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Zero_Price,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(0), TOfferTraits::OfferType}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenSellOfferMosaicNotAllowed) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Mosaic_Not_Allowed,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(Currency_Mosaic_Id), Amount(10)}, Amount(100), TOfferTraits::OfferType}, BlockDuration(100)},
			});
	}

	TRAITS_BASED_TEST(FailureWhenOfferAlreadyExists) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry entry(offerOwner, 1);
		TOfferTraits::InsertOffer(entry);

		// Assert:
		AssertValidationResult<TValidatorTraits>(
			Failure_Exchange_Offer_Exists,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, BlockDuration(100)},
			},
			offerOwner,
			&entry);
	}

	TRAITS_BASED_TEST(Success) {
		// Assert:
		AssertValidationResult<TValidatorTraits>(
			ValidationResult::Success,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(100)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(200)},
			},
			test::GenerateRandomByteArray<Key>());
	}

	TRAITS_BASED_TEST(Success_LongOffer) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult<TValidatorTraits>(
			ValidationResult::Success,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(1000)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(2000)},
			},
			offerOwner,
			nullptr,
			offerOwner);
	}
	
	TRAITS_BASED_TEST(SuccessWhenOfferDurationInsideMosaicDuration) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		
		// Assert:
		AssertValidationResultExpiringMosaic<TValidatorTraits>(
			ValidationResult::Success,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(50)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(50)},
			},
			offerOwner,
			nullptr,
			offerOwner);
	}
	
	TRAITS_BASED_TEST(FailureWhenOfferDurationExceedMosaicDuration) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		
		// Assert:
		AssertValidationResultExpiringMosaic<TValidatorTraits>(
			Failure_Exchange_Offer_Duration_Exceeds_Mosaic_Duration,
			{
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(1000)},
				model::OfferWithDuration{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(2000)},
			},
			offerOwner,
			nullptr,
			offerOwner);
	}
}}
