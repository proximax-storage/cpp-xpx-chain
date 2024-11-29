/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/state/AccountState.h"
#include "catapult/model/Address.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace validators {

#define TEST_CLASS ModifyStateValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ModifyState, )

	namespace {
		using Notification = model::ModifyStateNotification<1>;

		auto CreateAccountState(const Key& accountKey) {
			auto address = model::PublicKeyToAddress(accountKey, model::NetworkIdentifier::Mijin_Test);

			state::AccountState entry(address, Height(1));
			entry.Balances = state::AccountBalances(&entry);
			entry.Balances.credit(MosaicId(5), Amount(100));
			entry.PublicKey = accountKey;
			return entry;
		}

		void AssertValidationResult(
				ValidationResult expectedResult, cache::CacheId cacheId, const Key& accountKey, const Key& nemesisKey) {
			// Arrange:
			Height currentHeight(1);
			auto entry = CreateAccountState(accountKey);
			auto cache =  test::CoreSystemCacheFactory::Create();
			auto serializedEntry = cache::AccountStatePrimarySerializer::SerializeValue(entry);
			Notification notification(cacheId,
									  static_cast<uint8_t>(cache::GeneralSubCache::Main),
									  test::GenerateRandomByteArray<Key>(),
									  reinterpret_cast<const uint8_t*>(&entry.Address),
									  Address_Encoded_Size,
									  reinterpret_cast<const uint8_t*>(serializedEntry.data()),
									  serializedEntry.size());
			auto pValidator = CreateModifyStateValidator();
			auto config =config::BlockchainConfiguration::Uninitialized();
			*const_cast<Key*>(&config.Network.Info.PublicKey) = nemesisKey;
			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config, currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenCacheIsNotValid) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_ModifyState_Cache_Id_Invalid,
				cache::CacheId(999),
				key,
				key);
	}

	TEST(TEST_CLASS, FailureWhenKeyIsNotNemesis) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
				Failure_ModifyState_Signer_Not_Nemesis,
				cache::CacheId::AccountState,
				key,
				test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
				ValidationResult::Success,
				cache::CacheId::AccountState,
				key,
				key);
	}
}}
