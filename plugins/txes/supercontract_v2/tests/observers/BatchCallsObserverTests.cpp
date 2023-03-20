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
        const auto Digest =  model::ExtendedCallDigest{
                test::GenerateRandomByteArray<Hash256>(),
                true,
                Current_Height,
                0,
                test::GenerateRandomByteArray<Hash256>(),
        };
        const auto Digests = std::vector<model::ExtendedCallDigest>{Digest};
        const auto PaymentOpinions = std::vector<model::CallPaymentOpinion>{{
                std::vector<Amount>{Amount(10)},
                std::vector<Amount>{Amount(10)}
        }};
        const auto Replicators_Count = 3;

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(Super_Contract_Key);

			return entry;
		}

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();
            config.Immutable.SuperContractMosaicId = test::GenerateRandomValue<MosaicId>();
            config.Immutable.StreamingMosaicId = test::GenerateRandomValue<MosaicId>();

            return config.ToConst();
        }

        auto CreateContractCall() {
            state::ContractCall contractCall;
            contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
            contractCall.Caller = test::GenerateRandomByteArray<Key>();
            contractCall.FileName = test::GenerateRandomString(5);
            contractCall.FunctionName = test::GenerateRandomString(5);
            contractCall.ActualArguments = test::GenerateRandomString(5);
            contractCall.ExecutionCallPayment = PaymentOpinions[0].ExecutionWork[0]+Amount(1);
            contractCall.DownloadCallPayment = PaymentOpinions[0].ExecutionWork[0]+Amount(1);
            contractCall.BlockHeight = Current_Height;

            return contractCall;
        }

        auto CreateCompletedCall(Key& caller) {
            state::CompletedCall completedCall;
            completedCall.CallId = Digest.CallId;
            completedCall.Caller = caller;
            completedCall.Status = 0;
            completedCall.ExecutionWork = PaymentOpinions[0].ExecutionWork[0];
            completedCall.DownloadWork = PaymentOpinions[0].ExecutionWork[0];

            return completedCall;
        }

        struct CacheValues {
        public:
            CacheValues() : InitialScEntry(Key()), ExpectedScEntry(Key()) {}

        public:
            state::SuperContractEntry InitialScEntry;
            state::SuperContractEntry ExpectedScEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values, const std::vector<model::ExtendedCallDigest>& digests) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            auto notification = Notification(values.InitialScEntry.key(), digests,PaymentOpinions);
            auto pObserver = CreateBatchCallsObserver(Liquidity_Provider, Drive_Browser);

            const auto& scMosaicId = config::GetUnresolvedSuperContractMosaicId(context.observerContext().Config.Immutable);
            const auto& currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(context.observerContext().Config.Immutable);
            const auto& streamingMosaicId = config::GetUnresolvedStreamingMosaicId(context.observerContext().Config.Immutable);
            auto resolvedScMosaicId = MosaicId(scMosaicId.unwrap());
            auto resolvedCurrencyMosaicId = MosaicId(currencyMosaicId.unwrap());
            auto resolvedStreamingMosaicId = MosaicId(streamingMosaicId.unwrap());

            auto& accountCache = context.cache().sub<cache::AccountStateCache>();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

            // Populate cache.
            accountCache.addAccount(values.InitialScEntry.key(), Height(1));
            accountCache.addAccount(values.InitialScEntry.creator(), Height(1));

            auto initAmount = Amount(1000000000);
            if (mode != NotifyMode::Rollback) {
                accountCache.addAccount(values.InitialScEntry.requestedCalls().front().Caller, Height(1));

                test::AddAccountState(
                        accountCache,
                        values.InitialScEntry.executionPaymentKey(),
                        Height(1),
                        {
                                { resolvedScMosaicId, initAmount },
                                { resolvedStreamingMosaicId, initAmount },
                        });
            }

            state::BcDriveEntry bcDrive(values.InitialScEntry.driveKey());
            bcDrive.setReplicatorCount(Replicators_Count);
            bcDriveCache.insert(bcDrive);
            superContractCache.insert(values.InitialScEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto& actualScEntry = superContractCacheIter.get();
            test::AssertEqualSuperContractData(values.ExpectedScEntry, actualScEntry);

            auto& executionPaymentAcc = accountCache.find(values.InitialScEntry.executionPaymentKey()).get();
            EXPECT_TRUE(executionPaymentAcc.Balances.get(resolvedScMosaicId) < initAmount);
            EXPECT_TRUE(executionPaymentAcc.Balances.get(resolvedStreamingMosaicId) < initAmount);

            auto refundReceiver = digests[0].Manual ? values.InitialScEntry.requestedCalls().front().Caller : values.InitialScEntry.creator();
            auto refundReceiverEntry = accountCache.find(refundReceiver).get();
            EXPECT_TRUE(refundReceiverEntry.Balances.get(resolvedCurrencyMosaicId) > Amount(0));

            if (digests[0].Manual) {
                auto servicePaymentsReceiverKey = digests[0].Status == 0
                        ? values.InitialScEntry.key() : values.InitialScEntry.requestedCalls().front().Caller;
                auto servicePaymentsReceiverAccountEntry = accountCache.find(values.InitialScEntry.requestedCalls().front().Caller).get();

                for (const auto& [mosaicId, amount] : values.InitialScEntry.requestedCalls().front().ServicePayments) {
                    auto resolvedMosaicId = context.observerContext().Resolvers.resolve(mosaicId);
                    EXPECT_EQ(servicePaymentsReceiverAccountEntry.Balances.get(resolvedMosaicId), amount);
                }
            }
        }
	}

	TEST(TEST_CLASS, BatchCalls_Commit_Manual) {
        // Arrange
        CacheValues values;
        values.InitialScEntry = CreateSuperContractEntry();
        values.InitialScEntry.setExecutionPaymentKey(test::GenerateRandomByteArray<Key>());
        values.InitialScEntry.batches()[0] = {};
        values.InitialScEntry.automaticExecutionsInfo().AutomatedExecutionsNumber = 2;

        auto call = CreateContractCall();
        values.InitialScEntry.requestedCalls().emplace_back(call);

        values.ExpectedScEntry = values.InitialScEntry;
        values.ExpectedScEntry.batches()[0].CompletedCalls.emplace_back(CreateCompletedCall(call.Caller));
        values.ExpectedScEntry.releasedTransactions().insert(Digest.ReleasedTransactionHash);
        values.ExpectedScEntry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince = Current_Height;
        values.ExpectedScEntry.requestedCalls().clear();

        // Assert
        RunTest(NotifyMode::Commit, values, Digests);
	}

	TEST(TEST_CLASS, BatchCalls_Rollback) {
        // Arrange
        CacheValues values;

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Digests), catapult_runtime_error);
	}
}}
