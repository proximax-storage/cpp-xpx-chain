/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS EndFileDownloadObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndFileDownload,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DownloadCacheFactory>;
		using Notification = model::EndFileDownloadNotification<1>;

		constexpr MosaicId Streaming_Mosaic_Id(1234);
		constexpr auto File_Count = 4u;
		constexpr auto File_Size = 10u;

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;
			return config.ToConst();
		}

		struct DownloadValues {
		public:
			explicit DownloadValues()
				: FileRecipient(test::GenerateRandomByteArray<Key>())
				, OperationToken(test::GenerateRandomByteArray<Hash256>())
				, Files{
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
					model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() },
				}
			{}

		public:
			Key FileRecipient;
			Hash256 OperationToken;
			std::vector<model::DownloadAction> Files;
		};

		state::DownloadEntry CreateDownloadEntry(const DownloadValues& values, bool withFiles) {
			state::DownloadEntry entry(values.OperationToken);
			entry.FileRecipient = values.FileRecipient;
			if (withFiles) {
				for (const auto &file : values.Files)
					entry.Files.emplace(file.FileHash, file.FileSize);
			}
			return entry;
		}
	}

	TEST(TEST_CLASS, EndFileDownload_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Height(124), CreateConfiguration());
		DownloadValues values;
		Notification notification(values.FileRecipient, values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateEndFileDownloadObserver();
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();

		// Populate cache.
		downloadCache.insert(CreateDownloadEntry(values, true));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert:
		auto downloadIter = downloadCache.find(values.OperationToken);
		const auto& actualEntry = downloadIter.get();
		test::AssertEqualDownloadData(CreateDownloadEntry(values, false), actualEntry);
	}

	TEST(TEST_CLASS, EndFileDownload_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Height(124), CreateConfiguration());
		DownloadValues values;
		Notification notification(values.FileRecipient, values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateEndFileDownloadObserver();
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();

		// Populate cache.
		downloadCache.insert(CreateDownloadEntry(values, false));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto downloadIter = downloadCache.find(values.OperationToken);
		const auto& actualEntry = downloadIter.get();
		test::AssertEqualDownloadData(CreateDownloadEntry(values, true), actualEntry);
	}
}}
