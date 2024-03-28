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

#define TEST_CLASS UnsuccessfulEndBatchExecutionObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(UnsuccessfulEndBatchExecution,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::UnsuccessfulBatchExecutionNotification<1>;

        const auto Current_Height = test::GenerateRandomValue<Height>();
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto BatchId = 0;
        const auto AutomaticExecutionsNextBlockToCheck = Current_Height;
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
        };

        void RunTest(NotifyMode mode, const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            Notification notification(values.InitialScEntry.key(), BatchId, Cosigners);

            auto pObserver = CreateUnsuccessfulEndBatchExecutionObserver();
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
        batch.PoExVerificationInformation = crypto::CurvePoint();

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
