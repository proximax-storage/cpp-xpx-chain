/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SdaExchangeCache.h"
#include "src/config/SdaExchangeConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS PlaceSdaExchangeOfferValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(PlaceSdaExchangeOfferV1, )

    namespace {
        using MosaicSetup = std::function<void(cache::CatapultCacheDelta&)>;

        auto CreateConfig(const Key& longOfferKey) {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::SdaExchangeConfiguration::Uninitialized();
            pluginConfig.MaxOfferDuration = BlockDuration{500};
            pluginConfig.LongOfferKey = longOfferKey;
            config.Network.SetPluginConfiguration(pluginConfig);
            return (config.ToConst());
        }

        template<typename TTraits>
        void AssertValidationResultBase(
                ValidationResult expectedResult,
                MosaicSetup setup,
                const std::vector<model::SdaOfferWithDuration> offers = {},
                const Key& signer = test::GenerateRandomByteArray<Key>(),
                const state::SdaExchangeEntry* pEntry = nullptr,
                const Key& longOfferKey = test::GenerateRandomByteArray<Key>()) {
            // Arrange:
            Height currentHeight(1);
            auto cache = test::SdaExchangeCacheFactory::Create();
            
            auto delta = cache.createDelta();
            setup(delta);
            cache.commit(Height());
            
            if (pEntry) {
                auto& sdaExchangeDelta = delta.sub<cache::SdaExchangeCache>();
                sdaExchangeDelta.insert(*pEntry);
                cache.commit(currentHeight);
            }

            model::PlaceSdaOfferNotification<TTraits::Version> notification(signer, offers.size(), offers.data());
            auto config = CreateConfig(longOfferKey);
            auto pValidator = TTraits::CreateValidator();

            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }

        struct SdaOfferBalanceTraits {
            static void InsertOffer(state::SdaExchangeEntry& entry, const Height& deadline = Height(20)) {
                entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(100), Amount(10), Amount(100), Amount(10), deadline});
                entry.expiredSdaOfferBalances()[Height(10)].emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(100), Amount(10), Amount(100), Amount(10), deadline});
            }
        };

        struct ValidatorV1Traits {
            static constexpr VersionType Version = 1;
            static auto CreateValidator() {
                return CreatePlaceSdaExchangeOfferV1Validator();
            }
        };
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TOfferTraits, typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
    TEST(TEST_CLASS, TEST_NAME##_SdaOfferBalance_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SdaOfferBalanceTraits, ValidatorV1Traits>(); } \
    template<typename TOfferTraits, typename TValidatorTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

        TRAITS_BASED_TEST(FailureWhenNoSdaOffers) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_No_Offers);
        }

        TRAITS_BASED_TEST(FailureWhenExchangingSameUnits) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Exchanging_Same_Units_Is_Not_Allowed,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(1)), Amount(10)}}, BlockDuration(100)},
                },
                offerOwner,
                &entry);
        }

        TRAITS_BASED_TEST(FailureWhenDuplicatedSdaOffersInRequest) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Duplicated_Offer_In_Request,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(100)},
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(200)}, {test::UnresolveXor(MosaicId(2)), Amount(20)}}, BlockDuration(100)},
                },
                offerOwner,
                &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferDurationZero) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Zero_Offer_Duration,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(0)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferDurationTooLarge) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Offer_Duration_Too_Large,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(1000)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferZeroAmount) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Zero_Amount,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(0)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(100)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenOfferZeroPrice) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Zero_Price,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(0)}}, BlockDuration(100)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferAlreadyExists) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner, 1);
            TOfferTraits::InsertOffer(entry);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Offer_Exists,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(100)},
                },
                offerOwner,
                &entry);
        }

        TRAITS_BASED_TEST(Success) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner, 1);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                ValidationResult::Success,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(100)},
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(2)), Amount(200)}, {test::UnresolveXor(MosaicId(1)), Amount(10)}}, BlockDuration(200)},
                },
                offerOwner,
                &entry,
                offerOwner);
        }

        TRAITS_BASED_TEST(Success_LongOffer) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                ValidationResult::Success,
                {
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(1)), Amount(100)}, {test::UnresolveXor(MosaicId(2)), Amount(10)}}, BlockDuration(1000)},
                    model::SdaOfferWithDuration{model::SdaOffer{{test::UnresolveXor(MosaicId(2)), Amount(100)}, {test::UnresolveXor(MosaicId(1)), Amount(10)}}, BlockDuration(2000)},
                },
                offerOwner,
                &entry,
                offerOwner);
        }
}}