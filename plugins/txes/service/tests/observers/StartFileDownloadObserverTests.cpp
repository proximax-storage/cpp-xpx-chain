/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include <cmath>

namespace catapult { namespace observers {

#define TEST_CLASS StartFileDownloadObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(StartFileDownload,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DownloadCacheFactory>;
		using Notification = model::StartFileDownloadNotification<1>;

		struct DownloadValues {
		public:
			explicit DownloadValues()
				: DriveKey(test::GenerateRandomByteArray<Key>())
				, FileRecipient(test::GenerateRandomByteArray<Key>())
				, OperationToken(test::GenerateRandomByteArray<Hash256>())
				, Files{
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
				}
			{}

		public:
			Key DriveKey;
			Key FileRecipient;
			Hash256 OperationToken;
			std::vector<model::DownloadAction> Files;
		};

		state::DownloadEntry CreateEntry(const DownloadValues& values) {
			state::DownloadEntry entry(values.DriveKey);
			auto& fileHashes = entry.fileRecipients()[values.FileRecipient][values.OperationToken];
			for (const auto& file : values.Files)
				fileHashes.insert(file.FileHash);
			return entry;
		}
	}

	TEST(TEST_CLASS, StartFileDownload_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Height(1));
		DownloadValues values;
		Notification notification(values.DriveKey, values.FileRecipient, values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateStartFileDownloadObserver();

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();
		auto downloadIter = downloadCache.find(values.DriveKey);
		const auto& actualEntry = downloadIter.get();
		test::AssertEqualDownloadData(CreateEntry(values), actualEntry);
	}

	TEST(TEST_CLASS, StartFileDownload_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Height(1));
		DownloadValues values;
		Notification notification(values.DriveKey, values.FileRecipient, values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateStartFileDownloadObserver();
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();

		// Populate cache.
		downloadCache.insert(CreateEntry(values));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		EXPECT_FALSE(downloadCache.contains(values.DriveKey));
	}
}}
