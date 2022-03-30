/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <plugins/txes/mosaic/tests/test/MosaicCacheTestUtils.h>
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

        auto CreateConfig(const Key& longSdaOfferKey) {
            test::MutableBlockchainConfiguration config;
            auto pluginConfig = config::SdaExchangeConfiguration::Uninitialized();
            pluginConfig.MaxSdaOfferDuration = BlockDuration{500};
            pluginConfig.LongSdaOfferKey = longSdaOfferKey;
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
                const std::vector<model::SdaOfferWithOwnerAndDuration> offers = {},
                const Key& signer = test::GenerateRandomByteArray<Key>(),
                const state::SdaExchangeEntry* pEntry = nullptr,
                const Key& longSdaOfferKey = test::GenerateRandomByteArray<Key>()) {
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
            auto config = CreateConfig(longSdaOfferKey);
            auto pValidator = TTraits::CreateValidator();

            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

            // Assert:
            EXPECT_EQ(expectedResult, result);

            model::PlaceSdaOfferNotification<TTraits::Version> notification(signer, offers.size(), offers.data());
            auto config = CreateConfig(longSdaOfferKey);
            auto pValidator = TTraits::CreateValidator();

            // Act:
            auto result = test::ValidateNotification(*pValidator, notification, cache, config, currentHeight);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }

        template<typename TTraits>
        void AssertValidationResult(
            ValidationResult expectedResult,
            const std::vector<model::SdaOfferWithOwnerAndDuration> offers = {},
            const Key& signer = test::GenerateRandomByteArray<Key>(),
            const state::SdaExchangeEntry* pEntry = nullptr,
            const Key& longSdaOfferKey = test::GenerateRandomByteArray<Key>()) {
            
            AssertValidationResultBase<TTraits>(
                expectedResult,
                [](cache::CatapultCacheDelta& delta){
                    AddMosaicEternal(delta, MosaicId(1), Amount(100));
                    AddMosaicEternal(delta, MosaicId(2), Amount(100));
                }, offers, signer, pEntry, longSdaOfferKey);
        }

        template<typename TTraits>
        void AssertValidationResultExpiringMosaic(
            ValidationResult expectedResult,
            const std::vector<model::SdaOfferWithOwnerAndDuration> offers = {},
            const Key& signer = test::GenerateRandomByteArray<Key>(),
            const state::SdaExchangeEntry* pEntry = nullptr,
            const Key& longSdaOfferKey = test::GenerateRandomByteArray<Key>()) {
            
            AssertValidationResultBase<TTraits>(
                expectedResult,
                [](cache::CatapultCacheDelta& delta){
                    AddMosaic(delta, MosaicId(1), Height(1), BlockDuration(50), Amount(100));
                    AddMosaic(delta, MosaicId(2), Height(1), BlockDuration(50),Amount(100));
                }, offers, signer, pEntry, longSdaOfferKey);
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

        TRAITS_BASED_TEST(FailureWhenDuplicatedSdaOffersInRequest) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Duplicated_Offer_In_Request,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, Key(), BlockDuration(100)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, Key(), BlockDuration(100)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferDurationZero) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Zero_Offer_Duration,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, Key(), BlockDuration(0)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferDurationTooLarge) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Offer_Duration_Too_Large,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, Key(), BlockDuration(1000)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferZeroAmount) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Zero_Amount,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(0)}, {MosaicId(2), Amount(10)}}, Key(), BlockDuration(100)},
                });
        }

        TRAITS_BASED_TEST(FailureWhenOfferZeroPrice) {
            // Assert:
            AssertValidationResult<TValidatorTraits>(
                Failure_ExchangeSda_Zero_Price,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(0)}}, Key(), BlockDuration(100)},
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
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(100)},
                },
                offerOwner,
                &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferNotFound) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Offer_Doesnt_Exist,
                    {
                        model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(100)},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferExpired) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            TOfferTraits::InsertOffer(entry, Height(10));

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Offer_Expired,
                    {
                        model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(1)},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(FailureWhenNotEnoughSdaOfferUnits) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            TOfferTraits::InsertOffer(entry);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Not_Enough_Units_In_Offer,
                    {
                        model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(50)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(100)},
                    },
                    &entry);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferCantBeRemoved) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            TOfferTraits::InsertOffer(entry);

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                    Failure_ExchangeSda_Cant_Remove_Offer_At_Height,
                    {
                        model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(50)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(100)},
                    },
                    &entry);
        }
        
        TRAITS_BASED_TEST(Success) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            state::SdaExchangeEntry entry(offerOwner);
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(1), MosaicId(2)}, state::SdaOfferBalance{Amount(100), Amount(10), Amount(100), Amount(10), Height(20)});
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(2), MosaicId(1)}, state::SdaOfferBalance{Amount(200), Amount(10), Amount(200), Amount(10), Height(20)});

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                ValidationResult::Success,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, Key(), BlockDuration(100)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(2), Amount(200)}, {MosaicId(1), Amount(10)}}, Key(), BlockDuration(200)},
                },
                test::GenerateRandomByteArray<Key>());
        }

        TRAITS_BASED_TEST(Success_LongSdaOffer) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();

            // Assert:
            AssertValidationResult<TValidatorTraits>(
                ValidationResult::Success,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(1000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(2), Amount(100)}, {MosaicId(1), Amount(10)}}, offerOwner, BlockDuration(2000)},
                },
                offerOwner,
                nullptr,
                offerOwner);
        }

        TRAITS_BASED_TEST(SuccessWhenSdaOfferDurationInsideMosaicDuration) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            
            // Assert:
            AssertValidationResultExpiringMosaic<TValidatorTraits>(
                ValidationResult::Success,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(50)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(2), Amount(100)}, {MosaicId(1), Amount(10)}}, offerOwner, BlockDuration(50)},
                },
                offerOwner,
                nullptr,
                offerOwner);
        }

        TRAITS_BASED_TEST(FailureWhenSdaOfferDurationExceedMosaicDuration) {
            // Arrange:
            auto offerOwner = test::GenerateRandomByteArray<Key>();
            
            // Assert:
            AssertValidationResultExpiringMosaic<TValidatorTraits>(
                Failure_ExchangeSda_Offer_Duration_Exceeds_Mosaic_Duration,
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(1), Amount(100)}, {MosaicId(2), Amount(10)}}, offerOwner, BlockDuration(1000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{MosaicId(2), Amount(100)}, {MosaicId(1), Amount(10)}}, offerOwner, BlockDuration(2000)},
                },
                offerOwner,
                nullptr,
                offerOwner);
        }
}}