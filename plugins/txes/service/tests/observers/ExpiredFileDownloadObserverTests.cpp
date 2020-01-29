/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "plugins/txes/lock_shared/tests/observers/ExpiredLockInfoObserverTests.h"
#include "tests/test/DownloadCacheTestUtils.h"
#include "tests/test/ServiceTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS ExpiredFileDownloadObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ExpiredFileDownload,)

	namespace {
		struct ExpiredFileDownloadTraits : public test::BasicDownloadTestTraits {
		public:
			using ObserverTestContext = test::ObserverTestContextT<test::DownloadCacheFactory>;

		public:
			static auto CreateObserver() {
				return CreateExpiredFileDownloadObserver();
			}

			static auto& SubCache(cache::CatapultCacheDelta& cache) {
				return cache.sub<cache::DownloadCache>();
			}

			static Amount GetExpectedExpiringLockOwnerBalance(observers::NotifyMode mode, Amount initialBalance, Amount delta) {
				// expiring lock is paid to lock creator
				return observers::NotifyMode::Commit == mode ? initialBalance + delta : initialBalance - delta;
			}

			static Amount GetExpectedBlockSignerBalance(observers::NotifyMode, Amount initialBalance, Amount, size_t) {
				return initialBalance;
			}

			static auto GetLockOwner(const state::DownloadEntry& downloadEntry) {
				return downloadEntry.FileRecipient;
			}

			static ValueType CreateLockInfoWithAmount(MosaicId, Amount amount, Height height, cache::CatapultCacheDelta& cache) {
				auto downloadEntry = CreateLockInfo(height);
				downloadEntry.Files.clear();
				auto fileHash = test::GenerateRandomByteArray<Hash256>();
				downloadEntry.Files.insert(fileHash);
				state::DriveEntry driveEntry(downloadEntry.DriveKey);
				driveEntry.files().emplace(fileHash, state::FileInfo{ amount.unwrap() });
				auto& driveCache = cache.sub<cache::DriveCache>();
				driveCache.insert(driveEntry);
				return downloadEntry;
			}
		};
	}

	DEFINE_EXPIRED_LOCK_INFO_OBSERVER_TESTS(ExpiredFileDownloadTraits)
}}
