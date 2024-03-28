/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS AutomaticExecutionsReplenishmentObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(AutomaticExecutionsReplenishment,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::AutomaticExecutionsReplenishmentNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
		const auto Number = 10;

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(Super_Contract_Key);

			return entry;
		}

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
            auto notification = Notification(
                    values.ExpectedScEntry.key(),
                    values.ExpectedScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber);
            auto pObserver = CreateAutomaticExecutionsReplenishmentObserver();
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

            // Populate cache.
            superContractCache.insert(values.InitialScEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto& actualScEntry = superContractCacheIter.get();
            test::AssertEqualSuperContractData(values.ExpectedScEntry, actualScEntry);
        }
	}

	TEST(TEST_CLASS, AutomaticExecutionsReplenishment_Commit) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber = 0;
        values.ExpectedScEntry = values.InitialScEntry;
        values.ExpectedScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber = Number;

        // Assert
        RunTest(NotifyMode::Commit, values);
	}

    TEST(TEST_CLASS, AutomaticExecutionsReplenishment_Commit_ZeroNumber) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber = 0;
        values.ExpectedScEntry = values.InitialScEntry;

        // Assert
        RunTest(NotifyMode::Commit, values);
    }

    TEST(TEST_CLASS, AutomaticExecutionsReplenishment_Commit_DeploymentStatusIsCompleted) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialScEntry.batches()[0] = {};
        values.ExpectedScEntry = values.InitialScEntry;
        values.ExpectedScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber = Number;
        values.ExpectedScEntry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince = Current_Height;

        // Assert
        RunTest(NotifyMode::Commit, values);
    }

    TEST(TEST_CLASS, AutomaticExecutionsReplenishment_Commit_AutomaticExecutionsPrepaidSinceIsSet) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialScEntry.batches()[0] = {};
        values.InitialScEntry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince = Height(1);
        values.ExpectedScEntry = values.InitialScEntry;
        values.ExpectedScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber = Number;

        // Assert
        RunTest(NotifyMode::Commit, values);
    }

	TEST(TEST_CLASS, AutomaticExecutionsReplenishment_Rollback) {
        // Arrange
        CacheValues values;

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
