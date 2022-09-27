/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/LiquidityProviderTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS ManualRateChangeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ManualRateChange,)

    namespace {
        using Notification = model::ManualRateChangeNotification<1>;

        auto CreateConfig() {
        	test::MutableBlockchainConfiguration config;
        	config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();

        	auto lpConfig = config::LiquidityProviderConfiguration::Uninitialized();

        	config.Network.SetPluginConfiguration(lpConfig);

        	return config.ToConst();
        }

        constexpr auto Current_Height = Height(10);

		struct LiquidityProviderInfo {
			state::LiquidityProviderEntry entry;
			Amount currencyAmount;
			Amount mosaicAmount;
		};

        void AssertValidationResult(
                ValidationResult expectedResult,
				const UnresolvedMosaicId& mosaicId,
                const Key& signer,
                bool currencyBalanceIncrease,
                const Amount& currencyBalanceChange,
                bool mosaicBalanceIncrease,
                const Amount& mosaicBalanceChange,
				const std::optional<LiquidityProviderInfo>& info,
				const Amount& ownerCurrencyAmount) {
            // Arrange:
			auto config = CreateConfig();
            auto cache = test::LiquidityProviderCacheFactory::Create();
            {
            	auto currencyId = config.Immutable.CurrencyMosaicId;

                auto delta = cache.createDelta();
                auto& liquidityProviderCache = delta.sub<cache::LiquidityProviderCache>();
                auto& accountStateCache = delta.sub<cache::AccountStateCache>();

				if (info) {
					auto resolvedMosaicId = test::CreateResolverContextXor().resolve(info->entry.mosaicId());
					liquidityProviderCache.insert(info->entry);
					test::AddAccountState(
							accountStateCache,
							info->entry.providerKey(),
							Height(1),
							{ { currencyId, info->currencyAmount }, { resolvedMosaicId, info->mosaicAmount } });
				}

				test::AddAccountState(
						accountStateCache,
						signer,
						Height(1),
						{ { currencyId, ownerCurrencyAmount } });

                cache.commit(Current_Height);
            }

            Notification notification(signer, mosaicId, currencyBalanceIncrease, currencyBalanceChange, mosaicBalanceIncrease, mosaicBalanceChange);
            auto pValidator = CreateManualRateChangeValidator();

            // Act:
            auto result = test::ValidateNotification(
                *pValidator,
                notification,
                cache,
                config,
                Current_Height
            );

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

    TEST(TEST_CLASS, FailureWhenDoesNotExist) {
        // Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();


		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Amount>()};

        // Assert:sert:
		AssertValidationResult(
				Failure_LiquidityProvider_Liquidity_Provider_Is_Not_Registered,
				test::GenerateRandomValue<UnresolvedMosaicId>(),
				test::GenerateRandomByteArray<Key>(),
				test::RandomByte() % 2,
				test::GenerateRandomValue<Amount>(),
				test::RandomByte() % 2,
				test::GenerateRandomValue<Amount>(),
				info,
				test::GenerateRandomValue<Amount>());
	}

	TEST(TEST_CLASS, FailureWhenIsNotOwner) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Amount>()};

		// Assert:sert:
		AssertValidationResult(
				Failure_LiquidityProvider_Invalid_Owner,
				mosaicId,
				test::GenerateRandomByteArray<Key>(),
				test::RandomByte() % 2,
				test::GenerateRandomValue<Amount>(),
				test::RandomByte() % 2,
				test::GenerateRandomValue<Amount>(),
				info,
				test::GenerateRandomValue<Amount>());
	}

	TEST(TEST_CLASS, FailureWhenInsufficientSenderBalance) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();
		state::LiquidityProviderEntry entry(mosaicId);
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		LiquidityProviderInfo info = {entry, test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Amount>()};

		// Assert:sert:
		AssertValidationResult(
				Failure_LiquidityProvider_Insufficient_Currency,
				mosaicId,
				entry.owner(),
				true,
				Amount(1),
				test::RandomByte() % 2,
				test::GenerateRandomValue<Amount>(),
				info,
				Amount(0));
	}

	TEST(TEST_CLASS, FailureWhenInsufficientCurrencyLPBalance) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();
		state::LiquidityProviderEntry entry(mosaicId);
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		LiquidityProviderInfo info = {entry, Amount(0), test::GenerateRandomValue<Amount>()};

		// Assert:sert:
		AssertValidationResult(
				Failure_LiquidityProvider_Invalid_Exchange_Rate,
				mosaicId,
				entry.owner(),
				false,
				Amount(1),
				test::RandomByte() % 2,
				test::GenerateRandomValue<Amount>(),
				info,
				test::GenerateRandomValue<Amount>());
	}

	TEST(TEST_CLASS, FailureWhenInsufficientMosaicLPBalance) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();
		state::LiquidityProviderEntry entry(mosaicId);
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		LiquidityProviderInfo info = {entry, test::GenerateRandomValue<Amount>(), Amount(0)};

		// Assert:sert:
		AssertValidationResult(
				Failure_LiquidityProvider_Invalid_Exchange_Rate,
				mosaicId,
				entry.owner(),
				false,
				Amount(0),
				false,
				test::GenerateRandomValue<Amount>(),
				info,
				test::GenerateRandomValue<Amount>());
	}

	TEST(TEST_CLASS, Success) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();
		state::LiquidityProviderEntry entry(mosaicId);
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setProviderKey( { {1} } );

		LiquidityProviderInfo info = {entry, Amount(5), Amount(5)};

		// Assert:sert:
		AssertValidationResult(
				ValidationResult::Success,
				mosaicId,
				entry.owner(),
				false,
				Amount(1),
				false,
				Amount(1),
				info,
				Amount(5));
	}
}}