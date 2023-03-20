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
            CacheValues() : InitialScEntry(Key()), ExpectedScEntry(Key()) {}

        public:
            state::SuperContractEntry InitialScEntry;
            state::SuperContractEntry ExpectedScEntry;
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

            // Populate cache.
            superContractCache.insert(values.InitialScEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto actualScEntry = superContractCacheIter.tryGet();
            EXPECT_TRUE(actualScEntry);
            test::AssertEqualSuperContractData(values.ExpectedScEntry, *actualScEntry);
        }
	}

	TEST(TEST_CLASS, Commit) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.InitialScEntry.batches()[1] = {};

        values.ExpectedScEntry = values.InitialScEntry;
        auto& batch = values.InitialScEntry.batches()[1];
        batch.Success = false;
        batch.PoExVerificationInformation.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());

		// Assert
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, Rollback) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.ExpectedScEntry = values.InitialScEntry;

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
