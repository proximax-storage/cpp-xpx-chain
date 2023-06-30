/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "plugins/txes/mosaic/src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/LevyTestUtils.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult {
    namespace validators {

#define TEST_CLASS MosaicModifyLevyValidatorTests

        DEFINE_COMMON_VALIDATOR_TESTS(ModifyLevy,)

        /// region helper functions
        namespace {
            auto Unresolved_Mosaic_Id = UnresolvedMosaicId(1234);
            auto Currency_Mosaic_Id = MosaicId(Unresolved_Mosaic_Id.unwrap());
            using LevySetupFunc = std::function<void(cache::CatapultCacheDelta &, const Key &)>;

            model::MosaicLevyRaw CreateValidMosaicLevyRaw(
                    model::LevyType type = model::LevyType::Absolute,
                    UnresolvedAddress recipient = test::GenerateRandomByteArray<UnresolvedAddress>(),
                    UnresolvedMosaicId mosaicId = UnresolvedMosaicId(1000),
                    Amount fee = Amount(100)
            ) {
                return model::MosaicLevyRaw(type, recipient, mosaicId, fee);
            }

            model::MosaicProperties CreateMosaicProperties(model::MosaicFlags flags) {
                model::MosaicProperties::PropertyValuesContainer values{};
                values[utils::to_underlying_type(model::MosaicPropertyId::Flags)] = utils::to_underlying_type(flags);
                return model::MosaicProperties::FromValues(values);
            }

            void AddMosaic(
                    cache::CatapultCacheDelta &cache,
                    MosaicId id,
                    Height height,
                    Amount supply,
                    const Key &owner,
                    model::MosaicFlags flags
            ) {
                auto &mosaicCacheDelta = cache.sub<cache::MosaicCache>();
                auto definition = state::MosaicDefinition(height, owner, 1, CreateMosaicProperties(flags));
                auto entry = state::MosaicEntry(id, definition);
                entry.increaseSupply(supply);
                mosaicCacheDelta.insert(entry);
            }

            void SetupBaseMosaic(cache::CatapultCacheDelta &delta, const Key &signer) {
                AddMosaic(delta, Currency_Mosaic_Id, Height(1),
                          Amount(100), signer, model::MosaicFlags::Transferable);
            }

            void SetupLevyMosaic(cache::CatapultCacheDelta &delta, const Key &signer) {
                SetupBaseMosaic(delta, signer);
                AddMosaic(delta, MosaicId(333), Height(1), Amount(100), signer, model::MosaicFlags::None);
            }

            void AssertLevyParameterTest(
                    ValidationResult expectedResult,
                    const model::MosaicLevyRaw &levy,
                    bool levyEnable,
                    bool validRecipient,
                    const MosaicId &recipientBalanceMosaicId,
                    bool validSigner,
                    LevySetupFunc action
            ) {
                auto config = test::CreateMosaicConfigWithLevy(levyEnable);
                auto cache = test::LevyCacheFactory::Create(config);
                auto delta = cache.createDelta();
                auto owner = test::GenerateRandomByteArray<Key>();
                auto signer = test::GenerateRandomByteArray<Key>();

				auto& accountStateCache = delta.sub<cache::AccountStateCache>();
				accountStateCache.addAccount(owner, Height(1));
				accountStateCache.addAccount(signer, Height(1));
                auto xORLevy = model::MosaicLevyRaw(levy.Type, levy.Recipient,
                                                    test::UnresolveXor(MosaicId(levy.MosaicId.unwrap())), levy.Fee);
                auto notification = model::MosaicModifyLevyNotification<1>(test::UnresolveXor(Currency_Mosaic_Id),
                                                                           xORLevy, signer);

                action(delta, (validSigner) ? signer : owner);

                if (validRecipient) {
                    auto resolverContext = test::CreateResolverContextXor();
                    auto sinkResolved = resolverContext.resolve(levy.Recipient);
                    test::SetCacheBalances(delta, sinkResolved,
                                           test::CreateMosaicBalance(recipientBalanceMosaicId, Amount(1)));
                }

                cache.commit(Height());

                // Arrange:
                auto pValidator = CreateModifyLevyValidator();

                auto result = test::ValidateNotification(*pValidator, notification, cache, config);

                // Assert:
                EXPECT_EQ(expectedResult, result);
            }
        }
        /// end region

        // region validator tests

        TEST(TEST_CLASS, BaseMosaicIdNotFound) {
            AssertLevyParameterTest(
                    Failure_Mosaic_Id_Not_Found,
                    CreateValidMosaicLevyRaw(),
                    true,
                    false,
                    MosaicId(0),
                    false,
                    [](cache::CatapultCacheDelta &,
                       const Key &) {}
            );
        }

        TEST(TEST_CLASS, SignerIsInvalid) {
            AssertLevyParameterTest(
                    Failure_Mosaic_Ineligible_Signer,
                    CreateValidMosaicLevyRaw(),
                    true,
                    false,
                    MosaicId(0),
                    false,
                    SetupBaseMosaic
            );
        }

        TEST(TEST_CLASS, RecipientAddressIsInvalid) {
            AssertLevyParameterTest(
                    Failure_Mosaic_Recipient_Levy_Not_Exist,
                    CreateValidMosaicLevyRaw(),
                    true,
                    false,
                    MosaicId(0),
                    true,
                    SetupBaseMosaic
            );
        }

        TEST(TEST_CLASS, MosaicLevyIdNotFound) {
            auto recipient = test::GenerateRandomByteArray<Address>();

            AssertLevyParameterTest(
                    Failure_Mosaic_Levy_Not_Found_Or_Expired,
                    CreateValidMosaicLevyRaw(
                            model::LevyType::Absolute,
                            test::UnresolveXor(recipient),
                            UnresolvedMosaicId(1000)
                    ),
                    true,
                    true,
                    MosaicId(1000),
                    true,
                    SetupBaseMosaic
            );
        }

        TEST(TEST_CLASS, LevyFeeIsInvalid) {
            AssertLevyParameterTest(
                    Failure_Mosaic_Invalid_Levy_Fee,
                    CreateValidMosaicLevyRaw(
                            model::LevyType::Percentile,
                            test::GenerateRandomByteArray<UnresolvedAddress>(),
                            UnresolvedMosaicId(1000),
                            Amount(0)
                    ),
                    true,
                    true,
                    Currency_Mosaic_Id,
                    true,
                    SetupBaseMosaic
            );
        }

        TEST(TEST_CLASS, LevyFeeIsInvalidPercentile) {
            AssertLevyParameterTest(
                    Failure_Mosaic_Invalid_Levy_Fee,
                    CreateValidMosaicLevyRaw(
                            model::LevyType::Percentile,
                            test::GenerateRandomByteArray<UnresolvedAddress>(),
                            UnresolvedMosaicId(1000),
                            test::CreateMosaicLevyFeePercentile(300)
                    ),
                    true,
                    true,
                    Currency_Mosaic_Id,
                    true,
                    SetupBaseMosaic
            );
        }

        TEST(TEST_CLASS, NonTransferableLevy) {
            auto recipient = test::GenerateRandomByteArray<Address>();
            AssertLevyParameterTest(
                    Failure_Mosaic_Non_Transferable,
                    CreateValidMosaicLevyRaw(
                            model::LevyType::Absolute,
                            test::UnresolveXor(recipient),
                            UnresolvedMosaicId(333)
                    ),
                    true,
                    true,
                    Currency_Mosaic_Id,
                    true,
                    SetupLevyMosaic
            );
        }

        TEST(TEST_CLASS, AddMosaicLevyOK) {
            auto recipient = test::GenerateRandomByteArray<Address>();
            AssertLevyParameterTest(
                    ValidationResult::Success,
                    CreateValidMosaicLevyRaw(
                            model::LevyType::Absolute,
                            test::UnresolveXor(recipient),
                            Unresolved_Mosaic_Id
                    ),
                    true,
                    true,
                    Currency_Mosaic_Id,
                    true,
                    SetupBaseMosaic
            );
        }

        TEST(TEST_CLASS, AddMosaicLevyNotEnabled) {
            auto recipient = test::GenerateRandomByteArray<Address>();
            AssertLevyParameterTest(
                    Failure_Mosaic_Levy_Not_Enabled,
                    CreateValidMosaicLevyRaw(
                            model::LevyType::Absolute,
                            test::UnresolveXor(recipient),
                            Unresolved_Mosaic_Id
                    ),
                    false,
                    true,
                    Currency_Mosaic_Id,
                    true,
                    SetupBaseMosaic
            );
        }

        // endregion
    }
}
