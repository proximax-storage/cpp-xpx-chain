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

#define TEST_CLASS DebitMosaicValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DebitMosaic, std::make_unique<LiquidityProviderExchangeValidatorImpl>())

    namespace {
		using Notification = model::DebitMosaicNotification<1>;

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
				const LiquidityProviderInfo& info,
				const Amount& ownerMosaicAmount) {
            // Arrange:
			auto config = CreateConfig();
            auto cache = test::LiquidityProviderCacheFactory::Create();
            {
            	auto currencyId = config.Immutable.CurrencyMosaicId;

                auto delta = cache.createDelta();
                auto& liquidityProviderCache = delta.sub<cache::LiquidityProviderCache>();
                auto& accountStateCache = delta.sub<cache::AccountStateCache>();


				auto resolvedMosaicId = test::CreateResolverContextXor().resolve(info.entry.mosaicId());
				liquidityProviderCache.insert(info.entry);
				test::AddAccountState(
						accountStateCache,
						info.entry.providerKey(),
						Height(1),
						{ { currencyId, info.currencyAmount }, { resolvedMosaicId, info.mosaicAmount } });

				test::AddAccountState(
						accountStateCache,
						signer,
						Height(1),
						{ { resolvedMosaicId, ownerMosaicAmount } });

                cache.commit(Current_Height);
            }

            Notification notification(signer, test::GenerateRandomByteArray<Key>(), mosaicId, amount);

			std::unique_ptr<LiquidityProviderExchangeValidator> liquidityProvider = std::make_unique<LiquidityProviderExchangeValidatorImpl>();
			auto pValidator = CreateDebitMosaicValidator(liquidityProvider);

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
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomValue<UnresolvedMosaicId>(),
				test::GenerateRandomValue<Amount>(),
				info,
				test::GenerateRandomValue<Amount>());
	}

	TEST(TEST_CLASS, FailureWhenInsufficientMosaic) {
		// Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();

		LiquidityProviderInfo info = {state::LiquidityProviderEntry(mosaicId), test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Amount>()};

		// Assert:sert:
		AssertValidationResult(
				Failure_LiquidityProvider_Insufficient_Mosaic,
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

		// Assert:sert:
		AssertValidationResult(
				ValidationResult::Success,
				test::GenerateRandomByteArray<Key>(),
				mosaicId,
				Amount(1),
				info,
				Amount(100));
	}
}}