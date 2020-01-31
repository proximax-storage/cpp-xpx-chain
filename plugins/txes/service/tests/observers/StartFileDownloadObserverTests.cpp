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

		constexpr MosaicId Streaming_Mosaic_Id(1234);
		constexpr auto Current_Height = Height(10);
		constexpr auto Download_Duration = BlockDuration(20);

		auto CreateConfiguration() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;
			auto pluginConfig = config::ServiceConfiguration::Uninitialized();
			pluginConfig.DownloadDuration = Download_Duration;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}

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
			state::DownloadEntry entry(values.OperationToken);
			entry.DriveKey = values.DriveKey;
			entry.FileRecipient = values.FileRecipient;
			entry.Height = Current_Height + Height(Download_Duration.unwrap());
			for (const auto& file : values.Files)
				entry.Files.emplace(file.FileHash, file.FileSize);
			return entry;
		}
	}

	TEST(TEST_CLASS, StartFileDownload_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfiguration());
		DownloadValues values;
		Notification notification(values.DriveKey, values.FileRecipient, values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateStartFileDownloadObserver();

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert:
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();
		auto downloadIter = downloadCache.find(values.OperationToken);
		const auto& actualEntry = downloadIter.get();
		test::AssertEqualDownloadData(CreateEntry(values), actualEntry);

		auto pStatement = context.statementBuilder().build();
		ASSERT_EQ(1u, pStatement->TransactionStatements.size());
		const auto& receiptPair = *pStatement->TransactionStatements.find(model::ReceiptSource());
		ASSERT_EQ(1u, receiptPair.second.size());

		const auto& receipt = static_cast<const model::BalanceChangeReceipt&>(receiptPair.second.receiptAt(0));
		ASSERT_EQ(sizeof(model::BalanceChangeReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(model::Receipt_Type_Drive_Download_Started, receipt.Type);
		EXPECT_EQ(notification.FileRecipient, receipt.Account);
		EXPECT_EQ(Streaming_Mosaic_Id, receipt.MosaicId);
		uint64_t totalSize = 0u;
		auto pFile = notification.FilesPtr;
		for (auto i = 0u; i < notification.FileCount; ++i, ++pFile) {
			totalSize += pFile->FileSize;
		}
		EXPECT_EQ(Amount(totalSize), receipt.Amount);
	}

	TEST(TEST_CLASS, StartFileDownload_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height, CreateConfiguration());
		DownloadValues values;
		Notification notification(values.DriveKey, values.FileRecipient, values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateStartFileDownloadObserver();
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();

		// Populate cache.
		downloadCache.insert(CreateEntry(values));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert:
		EXPECT_FALSE(downloadCache.contains(values.OperationToken));

		auto pStatement = context.statementBuilder().build();
		ASSERT_EQ(0u, pStatement->TransactionStatements.size());
	}
}}
