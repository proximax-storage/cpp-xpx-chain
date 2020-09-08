/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS StartExecuteObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(StartExecute, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::StartExecuteNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();

		auto CreateSuperContractEntry(uint16_t executionCount) {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setExecutionCount(executionCount);

			return entry;
		}
	}

	TEST(TEST_CLASS, StartExecute_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		Notification notification(Super_Contract_Key);
		auto pObserver = CreateStartExecuteObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(11), superContractCacheIter.get());
	}

	TEST(TEST_CLASS, StartExecute_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		Notification notification(Super_Contract_Key);
		auto pObserver = CreateStartExecuteObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(9), superContractCacheIter.get());
	}
}}
