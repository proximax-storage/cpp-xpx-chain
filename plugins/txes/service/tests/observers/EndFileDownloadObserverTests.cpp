/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include <cmath>

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
				: DriveKey(test::GenerateRandomByteArray<Key>())
				, OperationToken(test::GenerateRandomByteArray<Hash256>())
				, Files{
					model::File{ test::GenerateRandomByteArray<Hash256>() },
					model::File{ test::GenerateRandomByteArray<Hash256>() },
					model::File{ test::GenerateRandomByteArray<Hash256>() },
					model::File{ test::GenerateRandomByteArray<Hash256>() },
				}
			{}

		public:
			Key DriveKey;
			Hash256 OperationToken;
			std::vector<model::File> Files;
		};

		state::DownloadEntry CreateDownloadEntry(const DownloadValues& values, bool withFiles) {
			state::DownloadEntry entry(values.OperationToken);
			entry.DriveKey = values.DriveKey;
			if (withFiles) {
				for (const auto &file : values.Files)
					entry.Files.insert(file.FileHash);
			}
			return entry;
		}

		state::DriveEntry CreateDriveEntry(const DownloadValues& values) {
			state::DriveEntry entry(values.DriveKey);
			for (const auto& file : values.Files)
				entry.files().emplace(file.FileHash, state::FileInfo{ File_Size });
			return entry;
		}
	}

	TEST(TEST_CLASS, EndFileDownload_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Height(124), CreateConfiguration());
		DownloadValues values;
		Notification notification(values.DriveKey, test::GenerateRandomByteArray<Key>(), values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateEndFileDownloadObserver();
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();

		// Populate cache.
		downloadCache.insert(CreateDownloadEntry(values, true));
		driveCache.insert(CreateDriveEntry(values));
		accountStateCache.addAccount(values.DriveKey, Height(1));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert:
		auto downloadIter = downloadCache.find(values.OperationToken);
		const auto& actualEntry = downloadIter.get();
		test::AssertEqualDownloadData(CreateDownloadEntry(values, false), actualEntry);

		test::AssertBalances(accountStateCache, values.DriveKey, { { Streaming_Mosaic_Id, Amount(File_Count * File_Size) } });

		auto pStatement = context.statementBuilder().build();
		ASSERT_EQ(1u, pStatement->TransactionStatements.size());
		const auto& receiptPair = *pStatement->TransactionStatements.find(model::ReceiptSource());
		ASSERT_EQ(1u, receiptPair.second.size());

		const auto& receipt = static_cast<const model::BalanceChangeReceipt&>(receiptPair.second.receiptAt(0));
		ASSERT_EQ(sizeof(model::BalanceChangeReceipt), receipt.Size);
		EXPECT_EQ(1u, receipt.Version);
		EXPECT_EQ(model::Receipt_Type_Drive_Download_Completed, receipt.Type);
		EXPECT_EQ(values.DriveKey, receipt.Account);
		EXPECT_EQ(Streaming_Mosaic_Id, receipt.MosaicId);
		EXPECT_EQ(Amount(File_Count * File_Size), receipt.Amount);
	}

	TEST(TEST_CLASS, EndFileDownload_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Height(124), CreateConfiguration());
		DownloadValues values;
		Notification notification(values.DriveKey, test::GenerateRandomByteArray<Key>(), values.OperationToken, values.Files.data(), values.Files.size());
		auto pObserver = CreateEndFileDownloadObserver();
		auto& downloadCache = context.cache().sub<cache::DownloadCache>();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();

		// Populate cache.
		downloadCache.insert(CreateDownloadEntry(values, false));
		driveCache.insert(CreateDriveEntry(values));
		test::SetCacheBalances(context.cache(), values.DriveKey, { { Streaming_Mosaic_Id, Amount(File_Count * File_Size) } });

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto downloadIter = downloadCache.find(values.OperationToken);
		const auto& actualEntry = downloadIter.get();
		test::AssertEqualDownloadData(CreateDownloadEntry(values, true), actualEntry);

		test::AssertBalances(accountStateCache, values.DriveKey, {});

		auto pStatement = context.statementBuilder().build();
		ASSERT_EQ(0u, pStatement->TransactionStatements.size());
	}
}}
