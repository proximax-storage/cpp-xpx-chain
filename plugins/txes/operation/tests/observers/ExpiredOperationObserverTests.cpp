/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "plugins/txes/lock_shared/tests/observers/ExpiredLockInfoObserverTests.h"
#include "tests/test/OperationTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS ExpiredOperationObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ExpiredOperation,)

	namespace {
		struct ExpiredOperationTraits : public test::BasicOperationTestTraits {
		public:
			using ObserverTestContext = test::ObserverTestContextT<test::OperationCacheFactory>;

		public:
			static auto CreateObserver() {
				return CreateExpiredOperationObserver();
			}

			static auto& SubCache(cache::CatapultCacheDelta& cache) {
				return cache.sub<cache::OperationCache>();
			}

			static Amount GetExpectedExpiringLockOwnerBalance(observers::NotifyMode mode, Amount initialBalance, Amount delta) {
				// expiring secret lock is paid to lock creator
				return observers::NotifyMode::Commit == mode ? initialBalance + delta : initialBalance - delta;
			}

			static Amount GetExpectedBlockSignerBalance(observers::NotifyMode, Amount initialBalance, Amount, size_t) {
				return initialBalance;
			}

			static auto GetLockOwner(const state::OperationEntry& entry) {
				return entry.Account;
			}

			static ValueType CreateLockInfoWithAmount(MosaicId mosaicId, Amount amount, Height height) {
				auto entry = CreateLockInfo(height);
				entry.Mosaics.clear();
				entry.Mosaics.emplace(mosaicId, amount);
				return entry;
			}
		};
	}

	DEFINE_EXPIRED_LOCK_INFO_OBSERVER_TESTS(ExpiredOperationTraits)
}}
