/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "src/model/OperationReceiptType.h"
#include "plugins/txes/lock_shared/tests/observers/LockObserverTests.h"
#include "tests/test/OperationNotificationsTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS StartOperationObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(StartOperation,)

	namespace {
		struct SecretObserverTraits {
		public:
			using CacheType = cache::OperationCache;
			using NotificationType = model::StartOperationNotification<1>;
			using NotificationBuilder = test::StartOperationNotificationBuilder;
			using ObserverTestContext = test::ObserverTestContextT<test::OperationCacheFactory>;

			static constexpr auto Debit_Receipt_Type = model::Receipt_Type_Operation_Started;

			static auto CreateObserver() {
				return CreateStartOperationObserver();
			}

			static auto GenerateRandomLockInfo(const NotificationType& notification) {
				auto entry = test::BasicOperationTestTraits::CreateLockInfo();
				entry.OperationToken = notification.OperationToken;
				return entry;
			}

			static auto ToKey(const NotificationType& notification) {
				return notification.OperationToken;
			}

			static void AssertAddedLockInfo(const state::OperationEntry& entry, const NotificationType& notification) {
				// Assert:
				ASSERT_EQ(notification.ExecutorCount, entry.Executors.size());
				auto pExecutor = notification.ExecutorsPtr;
				for (auto i = 0u; i < notification.ExecutorCount; ++i, ++pExecutor)
					EXPECT_EQ(1, entry.Executors.count(*pExecutor));
			}
		};
	}

	DEFINE_LOCK_OBSERVER_TESTS(SecretObserverTraits)
}}
