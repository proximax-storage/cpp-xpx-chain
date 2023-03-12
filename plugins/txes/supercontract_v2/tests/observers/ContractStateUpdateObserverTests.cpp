/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS ContractStateUpdateObserverTests

	const std::unique_ptr<state::DriveStateBrowser> Drive_Browser = std::make_unique<test::DriveStateBrowserImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(ContractStateUpdate, Drive_Browser);

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::ContractStateUpdateNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Drive_Key = test::GenerateRandomByteArray<Key>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
		const auto Super_Contract_Owner_Key = test::GenerateRandomByteArray<Key>();
		const auto File_Name = test::GenerateRandomString(10);
		const auto Function_Name = test::GenerateRandomString(10);
		const auto Execution_Call_Payment = Amount(10);
		const auto Download_Call_Payment = Amount(10);
		const auto Base_Modification_Id = Hash256();
        const auto Executors = std::vector<Key>{
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>(),
        };

		auto CreateDriveContractEntry() {
			auto driveEntry = test::CreateDriveContractEntry(Drive_Key, Super_Contract_Key);

			return driveEntry;
		}

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setDriveKey(Drive_Key);
			entry.setAssignee(Super_Contract_Owner_Key);
			entry.setExecutionPaymentKey(Super_Contract_Owner_Key);
			entry.setDeploymentBaseModificationId(Base_Modification_Id);

            auto& automaticExecutionsInfo = entry.automaticExecutionsInfo();
            automaticExecutionsInfo.AutomaticExecutionFileName = File_Name;
            automaticExecutionsInfo.AutomaticExecutionsFunctionName = Function_Name;
            automaticExecutionsInfo.AutomaticExecutionCallPayment = Execution_Call_Payment;
            automaticExecutionsInfo.AutomaticDownloadCallPayment = Download_Call_Payment;

            auto& executorsInfo = entry.executorsInfo();
            for (const auto &item: Executors) {
                executorsInfo[item] = state::ExecutorInfo();
            }

			return entry;
		}

		auto CreateNotification() {
			return Notification(Super_Contract_Key);
		}

        struct CacheValues {
        public:
            CacheValues() : ExpectedScEntry(Key()) {}

        public:
            state::SuperContractEntry ExpectedScEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            auto notification = CreateNotification();
            auto pObserver = CreateContractStateUpdateObserver(Drive_Browser);

            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
//            auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

            // Populate cache.
            superContractCache.insert(values.ExpectedScEntry);
            for (const auto &item: values.ExpectedScEntry.executorsInfo()) {
//                state::ReplicatorEntry replicatorEntry(item.first);
//                replicatorCache.insert(replicatorEntry);
            }

            if (mode == NotifyMode::Rollback) {

            }

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.ExpectedScEntry.key());
            const auto& actualScEntry = superContractCacheIter.get();
            test::AssertEqualSuperContractData(values.ExpectedScEntry, actualScEntry);
        }
	}

	TEST(TEST_CLASS, ContractStateUpdate_Commit) {
        // Arrange
        CacheValues values;
        values.ExpectedScEntry = CreateSuperContractEntry();

		// Assert
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, ContractStateUpdate_Rollback) {
        // Arrange
        CacheValues values;
        values.ExpectedScEntry = CreateSuperContractEntry();

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
