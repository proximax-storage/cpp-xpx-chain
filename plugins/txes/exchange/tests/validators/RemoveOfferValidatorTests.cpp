/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ExchangeCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS RemoveOfferValidatorTests

		DEFINE_COMMON_VALIDATOR_TESTS(RemoveOfferV1,)
		DEFINE_COMMON_VALIDATOR_TESTS(RemoveOfferV2,)

		namespace {
			template<typename TTraits>
			void AssertValidationResult(
					ValidationResult expectedResult,
					const Key& owner = test::GenerateRandomByteArray<Key>(),
					const std::vector<model::OfferMosaic> offers = {},
					const state::ExchangeEntry* pEntry = nullptr) {
				// Arrange:
				Height currentHeight(10);
				auto cache = test::ExchangeCacheFactory::Create();
				if (pEntry) {
					auto delta = cache.createDelta();
					auto& exchangeDelta = delta.sub<cache::ExchangeCache>();
					exchangeDelta.insert(*pEntry);
					cache.commit(currentHeight);
				}
				model::RemoveOfferNotification<TTraits::Version> notification(owner, offers.size(), offers.data());
				auto pValidator = TTraits::CreateValidator();

				// Act:
				auto result = test::ValidateNotification(*pValidator, notification, cache,
				                                         config::BlockchainConfiguration::Uninitialized(), currentHeight);

				// Assert:
				EXPECT_EQ(expectedResult, result);
			}

			struct SellOfferTraits {
				static constexpr auto OfferType = model::OfferType::Sell;
			};

			struct BuyOfferTraits {
				static constexpr auto OfferType = model::OfferType::Buy;
			};

			struct ValidatorV1Traits {
				static constexpr VersionType Version = 1;
				static auto CreateValidator() {
					return CreateRemoveOfferV1Validator();
				}
			};

			struct ValidatorV2Traits {
				static constexpr VersionType Version = 2;
				static auto CreateValidator() {
					return CreateRemoveOfferV2Validator();
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
					Failure_Exchange_No_Offered_Mosaics_To_Remove);
		}

		TRAITS_BASED_TEST(FailureWhenOwnerDoesntHaveAnyOffer) {
			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Account_Doesnt_Have_Any_Offer,
					test::GenerateRandomByteArray<Key>(),
					{
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), TOfferTraits::OfferType},
					});
		}

		TRAITS_BASED_TEST(FailureWhenDuplicatedOffersInRequest) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			entry.sellOffers().emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(20)}});
			entry.buyOffers().emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Height(20)}, Amount(100)});

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Duplicated_Offer_In_Request,
					offerOwner,
					{
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), TOfferTraits::OfferType},
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), TOfferTraits::OfferType},
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
					offerOwner,
					{
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), TOfferTraits::OfferType},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenOfferExpired) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			entry.sellOffers().emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(10)}});
			entry.buyOffers().emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Height(10)}, Amount(100)});

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Offer_Expired,
					offerOwner,
					{
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), TOfferTraits::OfferType},
					},
					&entry);
		}

		TRAITS_BASED_TEST(FailureWhenOfferAlreadyRemovedAtHeight) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::ExchangeEntry entry(offerOwner);
			entry.sellOffers().emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(20)}});
			entry.buyOffers().emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Height(20)}, Amount(100)});
			entry.expiredSellOffers()[Height(10)].emplace(MosaicId(1), state::SellOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(20)}});
			entry.expiredBuyOffers()[Height(10)].emplace(MosaicId(1), state::BuyOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Height(20)}, Amount(100)});

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					Failure_Exchange_Cant_Remove_Offer_At_Height,
					offerOwner,
					{
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), TOfferTraits::OfferType},
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
					offerOwner,
					{
							model::OfferMosaic{test::UnresolveXor(MosaicId(1)), model::OfferType::Sell},
							model::OfferMosaic{test::UnresolveXor(MosaicId(2)), model::OfferType::Buy},
					},
					&entry);
		}
	}}
