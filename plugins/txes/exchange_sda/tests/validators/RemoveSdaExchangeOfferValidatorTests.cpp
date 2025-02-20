/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SdaExchangeCache.h"
#include "src/validators/Validators.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS RemoveSdaExchangeOfferValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(RemoveSdaExchangeOffer, )

    namespace {
        template<typename TTraits>
            void AssertValidationResult(
                    ValidationResult expectedResult,
                    const Key& owner = test::GenerateRandomByteArray<Key>(),
                    const std::vector<model::SdaOfferMosaic> offers = {},
                    const state::SdaExchangeEntry* pEntry = nullptr) {
                // Arrange:
                Height currentHeight(10);
                auto cache = test::SdaExchangeCacheFactory::Create();
                if (pEntry) {
                    auto delta = cache.createDelta();
                    auto& exchangeDelta = delta.sub<cache::SdaExchangeCache>();
                    exchangeDelta.insert(*pEntry);
                    cache.commit(currentHeight);
                }
                model::RemoveSdaOfferNotification<TTraits::Version> notification(owner, offers.size(), offers.data());
                auto pValidator = TTraits::CreateValidator();

                // Act:
                auto result = test::ValidateNotification(*pValidator, notification, cache,
                                                         config::BlockchainConfiguration::Uninitialized(), currentHeight);

                // Assert:
                EXPECT_EQ(expectedResult, result);
            }

            struct ValidatorV1Traits {
                static constexpr VersionType Version = 1;
                static auto CreateValidator() {
                    return CreateRemoveSdaExchangeOfferValidator();
                }
            };
        }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
    TEST(TEST_CLASS, TEST_NAME##_SdaOfferMosaic_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ValidatorV1Traits>(); } \
    template<typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

        TRAITS_BASED_TEST(FailureWhenNoSdaOffers) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_No_Offered_Mosaics_To_Remove);
        }

        TRAITS_BASED_TEST(FailureWhenOwnerDoesntHaveAnySdaOffer) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Account_Doesnt_Have_Any_Offer,
                    test::GenerateRandomByteArray<Key>(),
                    {
                        model::SdaOfferMosaic{UnresolvedMosaicId(1), UnresolvedMosaicId(2)},
                    });
        }

        TRAITS_BASED_TEST(FailureWhenDuplicatedSdaOffersInRequest) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(10),  Amount(100), Amount(10), Amount(100), Height(20)});

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Duplicated_Offer_In_Request,
                    offerOwner,
                    {
                        model::SdaOfferMosaic{test::UnresolveXor(MosaicId(1)), test::UnresolveXor(MosaicId(2))},
                        model::SdaOfferMosaic{test::UnresolveXor(MosaicId(1)), test::UnresolveXor(MosaicId(2))},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferNotFound) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Offer_Doesnt_Exist,
                    offerOwner,
                    {
                        model::SdaOfferMosaic{UnresolvedMosaicId(1), UnresolvedMosaicId(2)},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferExpired) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(10),  Amount(100), Amount(10), Amount(100), Height(10)});

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Offer_Expired,
                    offerOwner,
                    {
                        model::SdaOfferMosaic{test::UnresolveXor(MosaicId(1)), test::UnresolveXor(MosaicId(2))},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferAlreadyRemovedAtHeight) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(10), Amount(100), Amount(10), Amount(100), Height(20)});
            entry.expiredSdaOfferBalances()[Height(10)].emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(10), Amount(100), Amount(10), Amount(100), Height(20)});

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Cant_Remove_Offer_At_Height,
                    offerOwner,
                    {
                        model::SdaOfferMosaic{test::UnresolveXor(MosaicId(1)), test::UnresolveXor(MosaicId(2))},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(Success) {
			// Arrange:
			auto offerOwner = test::GenerateRandomByteArray<Key>();
			state::SdaExchangeEntry entry(offerOwner);
			entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(10), Amount(100), Amount(10), Amount(100), Height(20)});
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(2), MosaicId(1)}, state::SdaOfferBalance{Amount(10), Amount(100), Amount(10), Amount(100), Height(20)});

			// Assert:
			AssertValidationResult<TValidatorTraits>(
					ValidationResult::Success,
					offerOwner,
					{
						model::SdaOfferMosaic{test::UnresolveXor(MosaicId(1)), test::UnresolveXor(MosaicId(2))},
                        model::SdaOfferMosaic{test::UnresolveXor(MosaicId(2)), test::UnresolveXor(MosaicId(1))},
					},
					&entry);
		}
}}