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

#define TEST_CLASS CreateLiquidityProviderValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(CreateLiquidityProvider,)

    namespace {
        using Notification = model::CreateLiquidityProviderNotification<1>;

        auto managerKey = test::GenerateRandomByteArray<Key>();

        auto CreateConfig() {
        	test::MutableBlockchainConfiguration config;
        	config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();

        	auto lpConfig = config::LiquidityProviderConfiguration::Uninitialized();

        	lpConfig.PercentsDigitsAfterDot = 2;
			lpConfig.ManagerPublicKeys = { managerKey };
			lpConfig.MaxWindowSize = 10;

        	config.Network.SetPluginConfiguration(lpConfig);

        	return config.ToConst();
        }

        constexpr auto Current_Height = Height(10);

        void AssertValidationResult(
                ValidationResult expectedResult,
				const UnresolvedMosaicId& mosaicId,
                const Key& signer,
				const Amount& currencyDeposit,
				const Amount& initialMosaicMinting,
				uint32_t slashingPeriod,
				uint16_t windowSize,
				const std::vector<state::LiquidityProviderEntry>& existingEntries) {
            // Arrange:
			auto config = CreateConfig();
            auto cache = test::LiquidityProviderCacheFactory::Create();
            {
                auto delta = cache.createDelta();
                auto& liquidityProviderCache = delta.sub<cache::LiquidityProviderCache>();

				for (const auto& entry: existingEntries) {
					liquidityProviderCache.insert(entry);
				}

                cache.commit(Current_Height);
            }
            Notification notification(test::GenerateRandomByteArray<Key>(), signer, mosaicId, currencyDeposit, initialMosaicMinting, slashingPeriod, windowSize, test::GenerateRandomByteArray<Key>(), test::Random16(), test::Random16());
            auto pValidator = CreateCreateLiquidityProviderValidator();

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

    TEST(TEST_CLASS, FailureWhenAlreadyExists) {
        // Arrange
		auto mosaicId = test::GenerateRandomValue<UnresolvedMosaicId>();
        state::LiquidityProviderEntry entry(mosaicId);

        // Assert:
		AssertValidationResult(
				Failure_LiquidityProvider_Liquidity_Provider_Already_Exists,
				mosaicId,
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomValue<Amount>(),
				test::GenerateRandomValue<Amount>(),
				5,
				5,
				{ entry });
	}

	TEST(TEST_CLASS, FailureWhenInvalidOwner) {
    	// Arrange

    	// Assert:
    	AssertValidationResult(
    			Failure_LiquidityProvider_Invalid_Owner,
    			test::GenerateRandomValue<UnresolvedMosaicId>(),
    			test::GenerateRandomByteArray<Key>(),
    			test::GenerateRandomValue<Amount>(),
    			test::GenerateRandomValue<Amount>(),
    			5,
    			5,
    			{});
    }

    TEST(TEST_CLASS, FailureWhenInvalidSlashinPeriod) {
    	// Arrange

    	// Assert:
    	AssertValidationResult(
    			Failure_LiquidityProvider_Invalid_Slashing_Period,
    			test::GenerateRandomValue<UnresolvedMosaicId>(),
    			managerKey,
    			test::GenerateRandomValue<Amount>(),
    			test::GenerateRandomValue<Amount>(),
    			0,
    			5,
    			{});
    }

    TEST(TEST_CLASS, FailureWhenInvalidWindowSize) {
    	// Arrange

    	// Assert:
    	AssertValidationResult(
    			Failure_LiquidityProvider_Invalid_Window_Size,
    			test::GenerateRandomValue<UnresolvedMosaicId>(),
    			managerKey,
    			test::GenerateRandomValue<Amount>(),
    			test::GenerateRandomValue<Amount>(),
    			5,
    			0,
    			{});
    }

    TEST(TEST_CLASS, FailureWhenInvalidCurrencyDeposit) {
    	// Arrange

    	// Assert:
    	AssertValidationResult(
    			Failure_LiquidityProvider_Invalid_Exchange_Rate,
    			test::GenerateRandomValue<UnresolvedMosaicId>(),
    			managerKey,
    			Amount(0),
    			test::GenerateRandomValue<Amount>(),
    			5,
    			5,
    			{});
    }

    TEST(TEST_CLASS, FailureWhenInvalidMosaicMinting) {
    	// Arrange

    	// Assert:
    	AssertValidationResult(
    			Failure_LiquidityProvider_Invalid_Exchange_Rate,
    			test::GenerateRandomValue<UnresolvedMosaicId>(),
    			managerKey,
    			test::GenerateRandomValue<Amount>(),
    			Amount(0),
    			5,
    			5,
    			{});
    }

	TEST(TEST_CLASS, Success) {
		// Arrange

		// Assert:
		AssertValidationResult(
				ValidationResult::Success,
				test::GenerateRandomValue<UnresolvedMosaicId>(),
				managerKey,
				test::GenerateRandomValue<Amount>(),
				Amount(1),
				5,
				5,
				{});
	}
}}