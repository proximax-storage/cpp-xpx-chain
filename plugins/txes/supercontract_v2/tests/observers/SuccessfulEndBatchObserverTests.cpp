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

#define TEST_CLASS SuccessfulEndBatchExecutionObserverTests

    const std::unique_ptr<observers::StorageExternalManagementObserver> Storage_External_Manager = std::make_unique<observers::StorageExternalManagementObserverImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(SuccessfulEndBatchExecution, Storage_External_Manager)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::SuccessfulBatchExecutionNotification<1>;

        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Drive_Key = test::GenerateRandomByteArray<Key>();
        const auto BatchId = 0;
        const auto Storage_Hash = test::GenerateRandomByteArray<Hash256>();
        const auto Used_Size_Bytes = 20;
        const auto Meta_Files_Size_Bytes = 10;
        const auto Verification_Information = test::GenerateRandomByteArray<std::array<uint8_t, 32>>();
        const std::set<Key> Cosigners = {
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>()
        };

        struct CacheValues {
        public:
            CacheValues() :
                InitialScEntry(Key()),
                ExpectedScEntry(Key()),
                InitialBcDriveEntry(Key()),
                ExpectedBcDriveEntry(Key()) {}

        public:
            state::SuperContractEntry InitialScEntry;
            state::SuperContractEntry ExpectedScEntry;
            state::BcDriveEntry InitialBcDriveEntry;
            state::BcDriveEntry ExpectedBcDriveEntry;
            bool UpdateStorageState;
        };

        void RunTest(NotifyMode mode, const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);

            auto verificationInformation = crypto::CurvePoint();
            verificationInformation.fromBytes(Verification_Information);
            Notification notification(
                    values.InitialScEntry.key(),
                    BatchId,
                    values.UpdateStorageState,
                    Storage_Hash,
                    Used_Size_Bytes,
                    Meta_Files_Size_Bytes,
                    verificationInformation,
                    Cosigners);

            auto pObserver = CreateSuccessfulEndBatchExecutionObserver(Storage_External_Manager);
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDriveEntry);
            superContractCache.insert(values.InitialScEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto actualScEntry = superContractCacheIter.tryGet();
            ASSERT_TRUE(actualScEntry);
            test::AssertEqualSuperContractData(values.ExpectedScEntry, *actualScEntry);

            auto bcDriveCacheIter = bcDriveCache.find(values.InitialBcDriveEntry.key());
            const auto actualBcDriveEntry = bcDriveCacheIter.tryGet();
            ASSERT_TRUE(actualBcDriveEntry);
            EXPECT_EQ(values.ExpectedBcDriveEntry.rootHash(), actualBcDriveEntry->rootHash());
            EXPECT_EQ(values.ExpectedBcDriveEntry.usedSizeBytes(), actualBcDriveEntry->usedSizeBytes());
            EXPECT_EQ(values.ExpectedBcDriveEntry.metaFilesSizeBytes(), actualBcDriveEntry->metaFilesSizeBytes());
            EXPECT_EQ(values.ExpectedBcDriveEntry.metaFilesSizeBytes(), actualBcDriveEntry->metaFilesSizeBytes());
            EXPECT_EQ(values.ExpectedBcDriveEntry.ownerManagement(), actualBcDriveEntry->ownerManagement());
            EXPECT_EQ(values.ExpectedBcDriveEntry.verification()->VerificationTrigger, actualBcDriveEntry->verification()->VerificationTrigger);
            EXPECT_EQ(values.ExpectedBcDriveEntry.verification()->Expiration, actualBcDriveEntry->verification()->Expiration);
            EXPECT_EQ(values.ExpectedBcDriveEntry.verification()->Duration, actualBcDriveEntry->verification()->Duration);
            EXPECT_EQ(values.ExpectedBcDriveEntry.verification()->Shards.size(), actualBcDriveEntry->verification()->Shards.size());
            if (values.UpdateStorageState) {
                EXPECT_NE(values.InitialBcDriveEntry.lastModificationId(), actualBcDriveEntry->lastModificationId());
            }
        }
	}

	TEST(TEST_CLASS, Commit_UpdateStorageState) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.InitialScEntry.setDriveKey(Drive_Key);
        values.InitialScEntry.batches()[1] = {};

        values.ExpectedScEntry = values.InitialScEntry;
        auto& batch = values.ExpectedScEntry.batches()[1];
        batch.Success = true;
        batch.PoExVerificationInformation.fromBytes(Verification_Information);

        values.InitialBcDriveEntry = state::BcDriveEntry(Drive_Key);
        values.InitialBcDriveEntry.verification() = state::Verification();

        values.ExpectedBcDriveEntry = values.InitialBcDriveEntry;
        values.ExpectedBcDriveEntry.setRootHash(Storage_Hash);
        values.ExpectedBcDriveEntry.setUsedSizeBytes(Used_Size_Bytes);
        values.ExpectedBcDriveEntry.setMetaFilesSizeBytes(Meta_Files_Size_Bytes);
        values.ExpectedBcDriveEntry.setOwnerManagement(state::OwnerManagement::PERMANENTLY_FORBIDDEN);
        values.ExpectedBcDriveEntry.verification().reset();

        values.UpdateStorageState = true;

		// Assert
		RunTest(NotifyMode::Commit, values);
	}

    TEST(TEST_CLASS, Commit_DoNotUpdateStorageState) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.InitialScEntry.setDriveKey(Drive_Key);
        values.InitialScEntry.batches()[1] = {};

        values.ExpectedScEntry = values.InitialScEntry;
        auto& batch = values.ExpectedScEntry.batches()[1];
        batch.Success = true;
        batch.PoExVerificationInformation.fromBytes(Verification_Information);

        values.InitialBcDriveEntry = state::BcDriveEntry(Drive_Key);
        values.InitialBcDriveEntry.verification() = state::Verification();
        values.ExpectedBcDriveEntry = values.InitialBcDriveEntry;

        values.UpdateStorageState = false;

        // Assert
        RunTest(NotifyMode::Commit, values);
    }

	TEST(TEST_CLASS, Rollback) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.InitialScEntry.setDriveKey(Drive_Key);
        values.ExpectedScEntry = values.InitialScEntry;

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
