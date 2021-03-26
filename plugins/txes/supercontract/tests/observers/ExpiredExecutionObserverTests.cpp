/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS ExpiredExecutionObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ExpiredExecution, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::BlockNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();

		auto CreateNotification() {
			return Notification(Key(), Key(), Timestamp(), Difficulty(), 0u, 0u, Hash256());
		}

		auto CreateOperationEntry(state::LockStatus status, const Height& height, std::set<Key> executors) {
			state::OperationEntry entry(test::GenerateRandomByteArray<Hash256>());
			entry.Status = status;
			entry.Height = height;
			entry.Executors = executors;

			return entry;
		}

		auto CreateSuperContractEntry(uint16_t executionCount) {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setExecutionCount(executionCount);

			return entry;
		}
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(NotifyMode mode); \
	TEST(TEST_CLASS, TEST_NAME##_Commit) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(NotifyMode::Commit); } \
	TEST(TEST_CLASS, TEST_NAME##_Rollback) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(NotifyMode::Rollback); } \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(NotifyMode mode)

	TRAITS_BASED_TEST(ObserverDoesNothingWhenNoOperationExpired) {
		// Arrange:
		ObserverTestContext context(mode, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateExpiredExecutionObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height - Height(1), { Super_Contract_Key }));
		operationCache.insert(CreateOperationEntry(state::LockStatus::Used, Current_Height - Height(1), { Super_Contract_Key }));
		operationCache.insert(CreateOperationEntry(state::LockStatus::Used, Current_Height, { Super_Contract_Key }));
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height + Height(1), { Super_Contract_Key }));
		operationCache.insert(CreateOperationEntry(state::LockStatus::Used, Current_Height + Height(1), { Super_Contract_Key }));
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(10), superContractCacheIter.get());
	}

	TRAITS_BASED_TEST(ObserverDoesNothingWhenInvalidExecutorCount) {
		// Arrange:
		ObserverTestContext context(mode, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateExpiredExecutionObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height, {}));
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height, { Super_Contract_Key, test::GenerateRandomByteArray<Key>() }));
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(10), superContractCacheIter.get());
	}

	TRAITS_BASED_TEST(ObserverDoesNothingWhenNoExecutionExpired) {
		// Arrange:
		ObserverTestContext context(mode, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateExpiredExecutionObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height, { test::GenerateRandomByteArray<Key>() }));
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(10), superContractCacheIter.get());
	}

	TEST(TEST_CLASS, ObserverDecrementsExecutionCountOnCommit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateExpiredExecutionObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height, { Super_Contract_Key }));
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(9), superContractCacheIter.get());
	}

	TEST(TEST_CLASS, ObserverIncrementsExecutionCountOnRollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto notification = CreateNotification();
		auto pObserver = CreateExpiredExecutionObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(state::LockStatus::Unused, Current_Height, { Super_Contract_Key }));
		superContractCache.insert(CreateSuperContractEntry(10));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(11), superContractCacheIter.get());
	}
}}
