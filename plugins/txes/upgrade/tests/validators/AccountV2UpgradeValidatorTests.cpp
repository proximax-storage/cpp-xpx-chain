/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS AccountV2UpgradeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AccountV2Upgrade)

	namespace {
		auto CreateConfigHolder(uint32_t accountVersion) {
			test::MutableBlockchainConfiguration config;
			config.Network.AccountVersion = accountVersion;
			config.Network.MinimumAccountVersion = 1;
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		template<typename TCacheModifier>
		void AssertValidationResult(
				ValidationResult expectedResult,
				uint32_t accountVersion,
				Key signer,
				Key newAccountPublicKey,
				TCacheModifier cacheModifier) {
			// Arrange:
			auto cache = test::CoreSystemCacheFactory::Create();
			auto delta = cache.createDelta();
			auto& accountCache = delta.sub<cache::AccountStateCache>();
			cacheModifier(accountCache);
			cache.commit(Height(1));
			model::AccountV2UpgradeNotification<1> notification(signer, newAccountPublicKey);
			auto pValidator = CreateAccountV2UpgradeValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfigHolder(accountVersion)->Config(), Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureV2AccountsAreNotAllowed) {
		// Assert:
		AssertValidationResult(
			Failure_BlockchainUpgrade_Account_Version_Not_Allowed,
			1,
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
			[](auto& ){});
	}

	TEST(TEST_CLASS, FailureSignerIsNonExistant) {
		// Assert:
		AssertValidationResult(
				Failure_BlockchainUpgrade_Account_Non_Existant,
				2,
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				[](auto& ){});
	}

	TEST(TEST_CLASS, FailureSignerAlreadyV2Account) {
		// Assert:

		auto signerKey = test::GenerateRandomByteArray<Key>();
		AssertValidationResult(
				Failure_BlockchainUpgrade_Account_Not_Upgradable,
				2,
				signerKey,
				test::GenerateRandomByteArray<Key>(),
				[signerKey](cache::AccountStateCacheDelta& delta){
					delta.addAccount(signerKey, Height(1), 2);
				});
	}

	TEST(TEST_CLASS, FailureSignerIsARemoteAccount) {
		// Assert:

		auto signerKey = test::GenerateRandomByteArray<Key>();
		AssertValidationResult(
				Failure_BlockchainUpgrade_Account_Not_Upgradable,
				2,
				signerKey,
				test::GenerateRandomByteArray<Key>(),
				[signerKey](cache::AccountStateCacheDelta& delta){
				  delta.addAccount(signerKey, Height(1), 1);
				  auto& accountState = delta.find(signerKey).get();
				  accountState.AccountType = state::AccountType::Remote;
				});
	}

	TEST(TEST_CLASS, FailureSignerIsALockedAccount) {
		// Assert:

		auto signerKey = test::GenerateRandomByteArray<Key>();
		AssertValidationResult(
				Failure_BlockchainUpgrade_Account_Not_Upgradable,
				2,
				signerKey,
				test::GenerateRandomByteArray<Key>(),
				[signerKey](cache::AccountStateCacheDelta& delta){
				  delta.addAccount(signerKey, Height(1), 1);
				  auto& accountState = delta.find(signerKey).get();
				  accountState.AccountType = state::AccountType::Locked;
				});
	}

	TEST(TEST_CLASS, FailureTargetAccountAlreadyExists) {
		// Assert:

		auto signerKey = test::GenerateRandomByteArray<Key>();
		auto targetKey = test::GenerateRandomByteArray<Key>();
		AssertValidationResult(
				Failure_BlockchainUpgrade_Account_Duplicate,
				2,
				signerKey,
				targetKey,
				[signerKey, targetKey](cache::AccountStateCacheDelta& delta){
				  delta.addAccount(signerKey, Height(1), 1);
				  delta.addAccount(targetKey, Height(1));;
				});
	}

	TEST(TEST_CLASS, Success) {
		// Assert:

		auto signerKey = test::GenerateRandomByteArray<Key>();
		AssertValidationResult(
				ValidationResult::Success,
				2,
				signerKey,
				test::GenerateRandomByteArray<Key>(),
				[signerKey](cache::AccountStateCacheDelta& delta){
				  delta.addAccount(signerKey, Height(1), 1);
				});
	}
}}
