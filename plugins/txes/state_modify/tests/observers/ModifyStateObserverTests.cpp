/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/plugins/AccountObserverTestContext.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/model/Address.h"

namespace catapult { namespace observers {

#define TEST_CLASS ModifyStateObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ModifyState, )

	namespace {
		using ObserverTestContext = test::AccountObserverTestContext;
		using Notification = model::ModifyStateNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto CacheId = cache::GetCacheNameFromCacheId(cache::CacheId::AccountState);
		const auto SubCacheId = static_cast<cache::SubCacheId>(cache::GeneralSubCache::Main);

		auto CreateAccountState() {
			auto key = test::GenerateRandomByteArray<Key>();
			auto address = model::PublicKeyToAddress(key, model::NetworkIdentifier::Mijin_Test);

			state::AccountState entry(address, Height(1));
			entry.Balances = state::AccountBalances(&entry);
			entry.Balances.credit(MosaicId(5), Amount(100));
			entry.PublicKey = key;
			return entry;
		}
	}

	TEST(TEST_CLASS, ProperlyPopulatesAccountCacheWithNewRecord) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto entry = CreateAccountState();
		auto serializedEntry = cache::AccountStatePrimarySerializer::SerializeValue(entry);
		Notification notification(cache::CacheId::AccountState,
				SubCacheId,
				test::GenerateRandomByteArray<Key>(),
				reinterpret_cast<const uint8_t*>(&entry.Address),
				Address_Encoded_Size,
			  reinterpret_cast<const uint8_t*>(serializedEntry.data()),
				serializedEntry.size());
		auto pObserver = CreateModifyStateObserver();
		auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = accountStateCache.find(entry.Address);
		test::AssertEqual(entry, superContractCacheIter.get());
	}

	TEST(TEST_CLASS, ProperlyUpdatedAccountCacheWithNewRecord) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto entry = CreateAccountState();

		auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
		accountStateCache.addAccount(entry);

		entry.Balances.credit(MosaicId(1231), Amount(500));

		auto serializedEntry = cache::AccountStatePrimarySerializer::SerializeValue(entry);
		Notification notification(cache::CacheId::AccountState,
								  SubCacheId,
								  test::GenerateRandomByteArray<Key>(),
								  reinterpret_cast<const uint8_t*>(&entry.Address),
								  Address_Encoded_Size,
								  reinterpret_cast<const uint8_t*>(serializedEntry.data()),
								  serializedEntry.size());

		auto pObserver = CreateModifyStateObserver();


		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = accountStateCache.find(entry.Address);
		test::AssertEqual(entry, superContractCacheIter.get());
	}
}}
