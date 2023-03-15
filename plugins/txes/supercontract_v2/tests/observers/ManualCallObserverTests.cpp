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

#define TEST_CLASS ManualCallObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ManualCall,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::ManualCallNotification<1>;

        const auto Current_Height = Height(1);
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Call_Id = test::GenerateRandomByteArray<Hash256>();
        const auto Caller = test::GenerateRandomByteArray<Key>();
        const auto File_Name = test::GenerateRandomString(10);
        const auto Function_Name = test::GenerateRandomString(10);
        const auto Actual_Arguments = test::GenerateRandomString(10);
        const auto Execution_Call_Payment = Amount(10);
        const auto Download_Call_Payment = Amount(10);
        const auto Service_Payments = std::vector<model::UnresolvedMosaic>{model::UnresolvedMosaic()};

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
            Notification notification(
                    Super_Contract_Key,
                    Call_Id,
                    Caller,
                    File_Name,
                    Function_Name,
                    Actual_Arguments,
                    Execution_Call_Payment,
                    Download_Call_Payment,
                    Service_Payments);

            auto pObserver = CreateManualCallObserver();
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
        values.ExpectedScEntry = values.InitialScEntry;

        state::ContractCall contractCall{
            Call_Id,
            Caller,
            File_Name,
            Function_Name,
            Actual_Arguments,
            Execution_Call_Payment,
            Download_Call_Payment,
            Service_Payments,
            Current_Height,
        };
        values.ExpectedScEntry.requestedCalls().emplace_back(contractCall);

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
