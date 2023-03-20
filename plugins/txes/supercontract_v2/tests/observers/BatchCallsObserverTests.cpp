/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS BatchCallsObserverTests

    const std::unique_ptr<observers::LiquidityProviderExchangeObserver> Liquidity_Provider = std::make_unique<test::LiquidityProviderExchangeObserverImpl>();
	const std::unique_ptr<state::DriveStateBrowser> Drive_Browser = std::make_unique<test::DriveStateBrowserImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(BatchCalls, {}, Drive_Browser)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::BatchCallsNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Digests = std::vector<model::ExtendedCallDigest>{
                model::ExtendedCallDigest{
                        test::GenerateRandomByteArray<Hash256>(),
                        true,
                        Current_Height,
                        1,
                        test::GenerateRandomByteArray<Hash256>(),
                }};
        const auto PaymentOpinions = std::vector<model::CallPaymentOpinion>();

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
                    values.InitialScEntry.key(),
                    Digests,
                    PaymentOpinions);
            auto pObserver = CreateBatchCallsObserver(Liquidity_Provider, Drive_Browser);
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

            // Populate cache.
            superContractCache.insert(values.InitialScEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto& actualScEntry = superContractCacheIter.get();
            test::AssertEqualSuperContractData(values.InitialScEntry, actualScEntry);
        }
	}

	TEST(TEST_CLASS, BatchCalls_Commit) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.ExpectedScEntry = values.InitialScEntry;

        // Assert
        RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, BatchCalls_Rollback) {
        // Arrange
        CacheValues values;

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}
