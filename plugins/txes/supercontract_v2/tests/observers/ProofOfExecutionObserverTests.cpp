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

#define TEST_CLASS ProofOfExecutionObserverTests

    const std::unique_ptr<observers::LiquidityProviderExchangeObserver> Liquidity_Provider = std::make_unique<test::LiquidityProviderExchangeObserverImpl>();
    const std::unique_ptr<observers::StorageExternalManagementObserver> Storage_External_Manager = std::make_unique<observers::StorageExternalManagementObserverImpl>();

	DEFINE_COMMON_OBSERVER_TESTS(ProofOfExecution, Liquidity_Provider, Storage_External_Manager)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::ProofOfExecutionNotification<1>;

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();
            config.Immutable.SuperContractMosaicId = test::GenerateRandomValue<MosaicId>();
            config.Immutable.StreamingMosaicId = test::GenerateRandomValue<MosaicId>();

            return config.ToConst();
        }

        const auto Current_Height = Height(123);
        const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();
        const auto Execution_Payment_Key = test::GenerateRandomByteArray<Key>();

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

        void RunTest(
                ObserverTestContext& context,
                const CacheValues& values,
                std::map<Key, model::ProofOfExecution>& proofs) {
            // Arrange:
            Notification notification(Super_Contract_Key, proofs);

            const auto& scMosaicId = config::GetUnresolvedSuperContractMosaicId(context.observerContext().Config.Immutable);
            const auto& currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(context.observerContext().Config.Immutable);
            const auto& streamingMosaicId = config::GetUnresolvedStreamingMosaicId(context.observerContext().Config.Immutable);
            auto resolvedScMosaicId = MosaicId(scMosaicId.unwrap());
            auto resolvedCurrencyMosaicId = MosaicId(currencyMosaicId.unwrap());
            auto resolvedStreamingMosaicId = MosaicId(streamingMosaicId.unwrap());

            auto pObserver = CreateProofOfExecutionObserver(Liquidity_Provider, Storage_External_Manager);
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
            auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

            // Populate cache.
            for (const auto& item: values.InitialBcDrive.replicators()) {
                accountCache.addAccount(item, Height(1));
            }
            bcDriveCache.insert(values.InitialBcDrive);
            superContractCache.insert(values.InitialScEntry);

            auto initScAmount = Amount(1000000000);
            auto initStreamingAmount = Amount(1000000000);
            test::AddAccountState(
                    accountCache,
                    values.InitialScEntry.executionPaymentKey(),
                    Height(1),
                    {
                        { resolvedScMosaicId, initScAmount },
                        { resolvedStreamingMosaicId, initStreamingAmount },
                    });

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto superContractCacheIter = superContractCache.find(values.InitialScEntry.key());
            const auto actualScEntry = superContractCacheIter.tryGet();
            ASSERT_TRUE(actualScEntry);
            test::AssertEqualSuperContractData(values.ExpectedScEntry, *actualScEntry);

            auto bcDriveCacheIter = bcDriveCache.find(values.InitialScEntry.driveKey());
            const auto actualBcDrive = bcDriveCacheIter.tryGet();
            ASSERT_TRUE(actualBcDrive);
            for (const auto& key: values.ExpectedBcDrive.replicators()) {
                EXPECT_EQ(actualBcDrive->confirmedStorageInfos().at(key).ConfirmedStorageSince,
                          values.ExpectedBcDrive.confirmedStorageInfos().at(key).ConfirmedStorageSince);
                EXPECT_EQ(actualBcDrive->confirmedUsedSizes().at(key),
                          values.ExpectedBcDrive.usedSizeBytes());
            }

            for (const auto& [key, _ ]: proofs) {
                auto& acc = accountCache.find(key).get();
                EXPECT_TRUE(acc.Balances.get(resolvedCurrencyMosaicId) > Amount(0));
            }

            auto& executionPaymentAcc = accountCache.find(values.InitialScEntry.executionPaymentKey()).get();
            EXPECT_TRUE(executionPaymentAcc.Balances.get(resolvedScMosaicId) < initScAmount);
            EXPECT_TRUE(executionPaymentAcc.Balances.get(resolvedStreamingMosaicId) < initStreamingAmount);
        }
	}

	TEST(TEST_CLASS, Commit) {
        // Arrange
        ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());

        auto batchesCount = 3;
        auto replicatorsCount = 2;
        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.InitialScEntry.setExecutionPaymentKey(Execution_Payment_Key);
        for (auto i = 0; i < batchesCount; i++) {
            values.InitialScEntry.batches()[i] = {
                true,
                {},
                std::vector<state::CompletedCall>{
                    {
                        state::CompletedCall{
                            test::GenerateRandomByteArray<Hash256>(),
                            {},
                            0,
                            Amount(10),
                            Amount(10),
                        }
                    },
                }
            };
        }

        std::map<Key, model::ProofOfExecution> proofs;
        uint64_t batchId = 0;
        values.InitialBcDrive = state::BcDriveEntry(values.InitialScEntry.driveKey());
        for (auto i = 0; i < replicatorsCount; i++) {
            Key replicator = test::GenerateRandomByteArray<Key>();
            values.InitialScEntry.executorsInfo().insert({replicator, {batchId}});
            values.InitialBcDrive.replicators().insert(replicator);

            model::ProofOfExecution proof;
            proof.StartBatchId = batchId;
            proof.T.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
            proof.R = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
            proof.F.fromBytes(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());
            proof.K = crypto::Scalar(test::GenerateRandomByteArray<std::array<uint8_t, 32>>());

            proofs[replicator] = proof;

            if (batchId < values.InitialScEntry.batches().size()) {
                batchId++;
            }
        }

        values.ExpectedScEntry = values.InitialScEntry;
        values.ExpectedBcDrive = values.InitialBcDrive;
        for (const auto& key : values.ExpectedBcDrive.replicators()) {
            values.ExpectedBcDrive.confirmedStorageInfos()[key].ConfirmedStorageSince = context.observerContext().Timestamp;
            values.ExpectedBcDrive.confirmedUsedSizes()[key] = values.ExpectedBcDrive.usedSizeBytes();

            auto& executor = values.ExpectedScEntry.executorsInfo()[key];
            executor.PoEx.StartBatchId = values.ExpectedScEntry.nextBatchId();
            executor.NextBatchToApprove = values.ExpectedScEntry.nextBatchId();
            executor.PoEx.StartBatchId = proofs[key].StartBatchId;
            executor.PoEx.T = proofs[key].T;
            executor.PoEx.R = proofs[key].R;
        }

		// Assert
		RunTest(context, values, proofs);
	}

	TEST(TEST_CLASS, Rollback) {
        // Arrange
        ObserverTestContext context(NotifyMode::Rollback, Current_Height, CreateConfig());

        CacheValues values;
        values.InitialScEntry = state::SuperContractEntry(Super_Contract_Key);
        values.ExpectedScEntry = values.InitialScEntry;

        std::map<Key, model::ProofOfExecution> proofs;
		// Assert
		EXPECT_THROW(RunTest(context, values, proofs), catapult_runtime_error);
	}
}}
