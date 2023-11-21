/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ExchangeValidatorTests

		DEFINE_COMMON_VALIDATOR_TESTS(ExchangeV1,)
		DEFINE_COMMON_VALIDATOR_TESTS(ExchangeV2,)

		namespace {
			template<typename TTraits>
			void AssertValidationResult(
					ValidationResult expectedResult,
					const std::vector<model::MatchedOffer> offers = {},
					const state::ExchangeEntry* pEntry = nullptr,
					const Key& signer = test::GenerateRandomByteArray<Key>()) {
				// Arrange:
				Height currentHeight(10);
				auto cache = test::ExchangeCacheFactory::Create();
				if (pEntry) {
					auto delta = cache.createDelta();
					auto& exchangeDelta = delta.sub<cache::ExchangeCache>();
					exchangeDelta.insert(*pEntry);
					cache.commit(currentHeight);
				}
				model::ExchangeNotification<TTraits::Version> notification(signer, offers.size(), offers.data());
				auto pValidator = TTraits::CreateValidator();

				// Act:
				auto result = test::ValidateNotification(*pValidator, notification, cache,
				                                         config::BlockchainConfiguration::Uninitialized(), currentHeight);

				// Assert:
				EXPECT_EQ(expectedResult, result);
			}

			struct SellOfferTraits {
				static constexpr auto OfferType = model::OfferType::Sell;
				static void InsertOffer(state::ExchangeEntry& entry, const Height& deadline = Height(20)) {
					entry.sellOffers().emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), deadline}});
					entry.expiredSellOffers()[Height(10)].emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), deadline}});
				}
			};

			struct BuyOfferTraits {
				static constexpr auto OfferType = model::OfferType::Buy;
				static void InsertOffer(state::ExchangeEntry& entry, const Height& deadline = Height(20)) {
					entry.buyOffers().emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), deadline}, Amount(100)});
					entry.expiredBuyOffers()[Height(10)].emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), deadline}, Amount(100)});
				}
			};

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
	template<typename TOfferTraits, typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Sell_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SellOfferTraits, ValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Buy_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BuyOfferTraits, ValidatorV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Sell_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SellOfferTraits, ValidatorV2Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Buy_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BuyOfferTraits, ValidatorV2Traits>(); } \
	template<typename TOfferTraits, typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

		TRAITS_BASED_TEST(FailureWhenNoMatchedOffers) {
			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_No_Offers);
		}

		TRAITS_BASED_TEST(FailureWhenBuyingOwnUnits) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Buying_Own_Units_Is_Not_Allowed,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, offerOwner},
					},
					nullptr,
					offerOwner);
		}

		TRAITS_BASED_TEST(FailureWhenOwnerDoesntHaveAnyOffer) {
			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Account_Doesnt_Have_Any_Offer,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, test::GenerateRandomByteArray<Key>()},
					});
		}

		TRAITS_BASED_TEST(FailureWhenDuplicatedOffersInRequest) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			TOfferTraits::InsertOffer(entry);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Duplicated_Offer_In_Request,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(5)}, Amount(50), TOfferTraits::OfferType}, offerOwner},
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(5)}, Amount(50), TOfferTraits::OfferType}, offerOwner},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenOfferNotFound) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Offer_Doesnt_Exist,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, offerOwner},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenOfferExpired) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			TOfferTraits::InsertOffer(entry, Height(10));

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Offer_Expired,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, offerOwner},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenOfferPriceInvalid) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			TOfferTraits::InsertOffer(entry);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Invalid_Price,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(5)}, Amount(20), TOfferTraits::OfferType}, offerOwner},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenNotEnoughOfferUnits) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			TOfferTraits::InsertOffer(entry);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Not_Enough_Units_In_Offer,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(20)}, Amount(200), TOfferTraits::OfferType}, offerOwner},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenOfferCantBeRemoved) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			TOfferTraits::InsertOffer(entry);

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Cant_Remove_Offer_At_Height,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), TOfferTraits::OfferType}, offerOwner},
					},
					&entry);
		}

		TRAITS_BASED_TEST(Success) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			entry.sellOffers().emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(20)}});
			entry.buyOffers().emplace(MosaicId(2), state::BuyOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Height(20)}, Amount(100)});

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					ValidationResult::Success,
					{
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(10)}, Amount(100), model::OfferType::Sell}, offerOwner},
							model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(10)}, Amount(100), model::OfferType::Buy}, offerOwner},
					},
					&entry);
		}
	}}
