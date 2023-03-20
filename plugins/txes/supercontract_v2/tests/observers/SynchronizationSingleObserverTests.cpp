/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "plugins/txes/storage/src/observers/StorageExternalManagementObserverImpl.h"

namespace catapult { namespace observers {

#define TEST_CLASS SynchronizationSingleObserverTests

    const std::unique_ptr<observers::StorageExternalManagementObserver> Storage_External_Manager = std::make_unique<observers::StorageExternalManagementObserverImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(SynchronizationSingle, Storage_External_Manager)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::SynchronizationSingleNotification<1>;

        const auto Current_Height = Height(1);
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Drive_Key = test::GenerateRandomByteArray<Key>();
        const auto Executor = test::GenerateRandomByteArray<Key>();

        struct CacheValues {
        public:
            CacheValues() :
            InitialScEntry(Key()),
            ExpectedScEntry(Key()),
            InitialBcDrive(Key()),
            ExpectedBcDrive(Key()) {}

        public:
            state::SuperContractEntry InitialScEntry;
            state::SuperContractEntry ExpectedScEntry;
            state::BcDriveEntry InitialBcDrive;
            state::BcDriveEntry ExpectedBcDrive;
        };

        void RunTest(ObserverTestContext& context, const CacheValues& values) {
            // Arrange:
            Notification notification(Super_Contract_Key, values.InitialScEntry.nextBatchId(), Executor);

            auto pObserver = CreateSynchronizationSingleObserver(Storage_External_Manager);
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDrive);
            superContractCache.insert(values.InitialScEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto actualScEntry = superContractCacheIter.tryGet();
            EXPECT_TRUE(actualScEntry);
            test::AssertEqualSuperContractData(values.ExpectedScEntry, *actualScEntry);

            auto bcDriveCacheIter = bcDriveCache.find(values.InitialScEntry.driveKey());
            const auto actualBcDriveEntry = bcDriveCacheIter.get();
            EXPECT_EQ(actualBcDriveEntry.confirmedStorageInfos().at(Executor).ConfirmedStorageSince,
                      values.ExpectedBcDrive.confirmedStorageInfos().at(Executor).ConfirmedStorageSince);
            EXPECT_EQ(actualBcDriveEntry.confirmedUsedSizes().at(Executor),
                      values.ExpectedBcDrive.usedSizeBytes());
        }
	}

	TEST(TEST_CLASS, Commit) {
        // Arrange
        ObserverTestContext context(NotifyMode::Commit, Current_Height);

        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.InitialScEntry.setDriveKey(Drive_Key);
        values.InitialScEntry.executorsInfo()[Executor] = {};
        values.InitialScEntry.batches()[0] = {};
        values.InitialScEntry.batches()[1] = {};

        values.ExpectedScEntry = values.InitialScEntry;
        auto& executor = values.ExpectedScEntry.executorsInfo()[Executor];
        executor.NextBatchToApprove = values.InitialScEntry.nextBatchId();
        executor.PoEx.StartBatchId = values.InitialScEntry.nextBatchId();
        executor.PoEx.T = crypto::CurvePoint();
        executor.PoEx.R = crypto::Scalar();

        values.InitialBcDrive = state::BcDriveEntry(values.InitialScEntry.driveKey());
        for (const auto& item: values.InitialScEntry.executorsInfo()) {
            values.InitialBcDrive.replicators().insert(item.first);
        }
        values.ExpectedBcDrive.setUsedSizeBytes(10);

        values.ExpectedBcDrive = values.InitialBcDrive;
        values.ExpectedBcDrive.confirmedStorageInfos()[Executor].ConfirmedStorageSince = context.observerContext().Timestamp;
        values.ExpectedBcDrive.confirmedUsedSizes()[Executor] = values.InitialBcDrive.usedSizeBytes();

		// Assert
		RunTest(context, values);
	}

	TEST(TEST_CLASS, Rollback) {
        // Arrange
        ObserverTestContext context(NotifyMode::Rollback, Current_Height);

        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.ExpectedScEntry = values.InitialScEntry;

		// Assert
		EXPECT_THROW(RunTest(context, values), catapult_runtime_error);
	}
}}
