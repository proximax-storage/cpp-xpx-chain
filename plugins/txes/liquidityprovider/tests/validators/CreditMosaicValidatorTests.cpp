/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/LiquidityProviderTestUtils.h"
#include "src/validators/LiquidityProviderExchangeValidatorImpl.h"

namespace catapult { namespace validators {

#define TEST_CLASS CreditMosaicValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(CreditMosaic, LiquidityProviderExchangeValidatorImpl())

    namespace {
        using Notification = model::CreditMosaicNotification<1>;

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
                const Key& signer,
				const UnresolvedMosaicId& mosaicId,
                const Amount& amount,
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

            Notification notification(signer, test::GenerateRandomByteArray<Key>(), mosaicId, amount);

			LiquidityProviderExchangeValidatorImpl liquidityProvider;
			auto pValidator = CreateCreditMosaicValidator(liquidityProvider);

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

        // Assert:
		AssertValidationResult(
				Failure_LiquidityProvider_Liquidity_Provider_Is_Not_Registered,
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomValue<UnresolvedMosaicId>(),
				test::GenerateRandomValue<Amount>(),
				info,
				test::GenerateRandomValue<Amount>());
	}

	TEST(TEST_CLASS, FailureWhenInvalidMosaicAmount) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), Amount{1}, Amount{ULLONG_MAX}};

		// Assert:
		AssertValidationResult(
				Failure_LiquidityProvider_Invalid_Mosaic_Amount,
				test::GenerateRandomByteArray<Key>(),
				mosaicId,
				test::GenerateRandomValue<Amount>(),
				info,
				Amount{ULLONG_MAX});
	}

	TEST(TEST_CLASS, FailureWhenCurrencyOverflow) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), Amount{ULLONG_MAX}, Amount{1}};

		// Assert:
		AssertValidationResult(
				Failure_LiquidityProvider_Invalid_Currency_Amount,
				test::GenerateRandomByteArray<Key>(),
				mosaicId,
				Amount{2},
				info,
				Amount{ULLONG_MAX});
	}

	TEST(TEST_CLASS, FailureWhenInsufficientCurrency) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), Amount(1), Amount(1)};

		// Assert:
		AssertValidationResult(
				Failure_LiquidityProvider_Insufficient_Currency,
				test::GenerateRandomByteArray<Key>(),
				mosaicId,
				test::GenerateRandomValue<Amount>(),
				info,
				Amount(0));
	}

	TEST(TEST_CLASS, Success) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), Amount(1),Amount(1)};

		// Assert:
		AssertValidationResult(
				ValidationResult::Success,
				test::GenerateRandomByteArray<Key>(),
				mosaicId,
				Amount(1),
				info,
				Amount(100));
	}
}}