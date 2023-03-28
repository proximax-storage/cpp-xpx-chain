/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"
#include <catapult/cache/ReadOnlyCatapultCache.h>

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntrySerializer(Key key,
																 int contractCallCount,
																 int servicePaymentCount,
																 int executorCount,
																 int batchCount,
																 int completedCallCount,
																 int releaseTransactionCount) {
		state::SuperContractEntry entry(key);
		entry.setCreator( test::GenerateRandomByteArray<Key>() );
		entry.setDriveKey( test::GenerateRandomByteArray<Key>() );
		entry.setExecutionPaymentKey( test::GenerateRandomByteArray<Key>() );
		entry.setAssignee( test::GenerateRandomByteArray<Key>() );
		entry.setDeploymentBaseModificationId( test::GenerateRandomByteArray<Hash256>() );

		// poex
		crypto::Scalar scalar(std::array<uint8_t ,32>{5});
		crypto::CurvePoint curvePoint = crypto::CurvePoint::BasePoint() * scalar;

		// automatic executions info
		entry.automaticExecutionsInfo().AutomaticExecutionsFileName = "aaa";
		entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName = "aaa";
		entry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck = Height(1);
		entry.automaticExecutionsInfo().AutomaticExecutionCallPayment = Amount(1);
		entry.automaticExecutionsInfo().AutomaticDownloadCallPayment = Amount(1);
		entry.automaticExecutionsInfo().AutomatedExecutionsNumber = 0U;

		// contract call
		state::ServicePayment servicePayment;
		servicePayment.Amount = Amount(10);
		servicePayment.MosaicId = UnresolvedMosaicId(1);
		state::ContractCall contractCall;
		contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
		contractCall.Caller = test::GenerateRandomByteArray<Key>();
		contractCall.FileName = "aaa";
		contractCall.FunctionName = "aaa";
		contractCall.ActualArguments = "aaa";
		contractCall.ExecutionCallPayment = Amount(10);
		contractCall.DownloadCallPayment = Amount(10);
		contractCall.BlockHeight = Height(10);
		for(int i=0; i<servicePaymentCount; i++){
			contractCall.ServicePayments.push_back(servicePayment);  // service payment x3
		}
		for(int i=0; i<contractCallCount; i++){
			entry.requestedCalls().push_back(contractCall);  // contract call x3
		}

		// executors info
		state::ExecutorInfo executorInfo;
		executorInfo.PoEx.R = scalar;
		executorInfo.PoEx.T = curvePoint;
		for(int i=0; i<executorCount; i++){
			Key executor = test::GenerateRandomByteArray<Key>();
			entry.executorsInfo()[executor] = executorInfo;  // executors info x3
		}

		// batches
		state::CompletedCall completedCall;
		completedCall.CallId = test::GenerateRandomByteArray<Hash256>();
		completedCall.Caller = test::GenerateRandomByteArray<Key>();
		completedCall.Status = 0;
		completedCall.DownloadWork = Amount(10);
		completedCall.ExecutionWork = Amount(10);
		state::Batch batch;
		batch.Success = false;
		batch.PoExVerificationInformation = curvePoint;
		for(int i=0; i<completedCallCount; i++){
			batch.CompletedCalls.push_back(completedCall);  // completed call x3
		}
		for(int i=0; i<batchCount; i++){
			entry.batches()[i] = batch;  // batch x3
		}

		// released transactions
		for(int i=0; i<releaseTransactionCount; i++){
			Hash256 hash = test::GenerateRandomByteArray<Hash256>();
			entry.releasedTransactions().emplace(hash);  // released transaction x3
		}
		return entry;
	}

	state::SuperContractEntry CreateSuperContractEntry(
		Key superContractKey,
		Key driveKey,
		Key superContractOwnerKey,
		Key executionPaymentKey,
		Key creatorKey,
		Hash256 deploymentBaseModificationId) {
		state::SuperContractEntry entry(superContractKey);
		entry.setDriveKey(driveKey);
		entry.setAssignee(superContractOwnerKey);
		entry.setExecutionPaymentKey(executionPaymentKey);
		entry.setCreator(creatorKey);
		entry.setDeploymentBaseModificationId(deploymentBaseModificationId);

		return entry;
	}

	state::DriveContractEntry CreateDriveContractEntry(Key drive, Key contract) {
		state::DriveContractEntry entry(drive);
		entry.setContractKey(contract);

		return entry;
	}

	void AssertEqualAutomaticExecutionsInfo(const state::AutomaticExecutionsInfo& entry1, const state::AutomaticExecutionsInfo& entry2) {
		EXPECT_EQ(entry1.AutomatedExecutionsNumber, entry2.AutomatedExecutionsNumber);
		EXPECT_EQ(entry1.AutomaticDownloadCallPayment, entry2.AutomaticDownloadCallPayment);
		EXPECT_EQ(entry1.AutomaticExecutionCallPayment, entry2.AutomaticExecutionCallPayment);
		EXPECT_EQ(entry1.AutomaticExecutionsFileName, entry2.AutomaticExecutionsFileName);
		EXPECT_EQ(entry1.AutomaticExecutionsFunctionName, entry2.AutomaticExecutionsFunctionName);
		EXPECT_EQ(entry1.AutomaticExecutionsNextBlockToCheck, entry2.AutomaticExecutionsNextBlockToCheck);
		EXPECT_EQ(entry1.AutomaticExecutionsPrepaidSince, entry2.AutomaticExecutionsPrepaidSince);
	}

	void AssertEqualServicePayments(const std::vector<state::ServicePayment>& entry1, const std::vector<state::ServicePayment>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		if (entry1.empty())
			return;

		for (auto i = 0u; i < entry1.size(); i++) {
			const auto& expectedPayment = entry1[i];
			const auto& actualPayment = entry2[i];
			EXPECT_EQ(expectedPayment.Amount, actualPayment.Amount);
			EXPECT_EQ(expectedPayment.MosaicId, actualPayment.MosaicId);
		}
	}

	void AssertEqualExecutorsInfo(const std::map<Key, state::ExecutorInfo>& entry1, const std::map<Key, state::ExecutorInfo>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		if (entry1.empty())
			return;

		for (const auto& it : entry1){
			auto actualEntry = entry2.find(it.first);
			ASSERT_TRUE(actualEntry != entry2.end());

			EXPECT_EQ(it.second.NextBatchToApprove, actualEntry->second.NextBatchToApprove);
			EXPECT_EQ(it.second.PoEx.R, actualEntry->second.PoEx.R);
			EXPECT_EQ(it.second.PoEx.T, actualEntry->second.PoEx.T);
			EXPECT_EQ(it.second.PoEx.StartBatchId, actualEntry->second.PoEx.StartBatchId);
		}
	}

	void AssertEqualBatches(const std::map<uint64_t, state::Batch>& expectedBatches, const std::map<uint64_t, state::Batch>& actualBatches) {
		ASSERT_EQ(expectedBatches.size(), actualBatches.size());
		if (expectedBatches.empty())
			return;

		for (const auto& it : expectedBatches) {
			auto actualEntry = actualBatches.find(it.first);
			ASSERT_TRUE(actualEntry != actualBatches.end());

			EXPECT_EQ(it.second.Success, actualEntry->second.Success);
			EXPECT_EQ(it.second.PoExVerificationInformation, actualEntry->second.PoExVerificationInformation);

            auto& expectedCompletedCalls = it.second.CompletedCalls;
            auto& actualCompletedCalls = actualEntry->second.CompletedCalls;
			ASSERT_EQ(expectedCompletedCalls.size(), actualCompletedCalls.size());
			if (expectedCompletedCalls.empty())
				continue;

			for (auto i = 0u; i < expectedCompletedCalls.size(); i++) {
				const auto& expectedCall = expectedCompletedCalls[i];
				const auto& actualCall = actualCompletedCalls[i];
				EXPECT_EQ(expectedCall.CallId, actualCall.CallId);
				EXPECT_EQ(expectedCall.Caller, actualCall.Caller);
				EXPECT_EQ(expectedCall.DownloadWork, actualCall.DownloadWork);
				EXPECT_EQ(expectedCall.ExecutionWork, actualCall.ExecutionWork);
				EXPECT_EQ(expectedCall.Status, actualCall.Status);
			}
		}
	}

	void AssertEqualRequestedCalls(const std::deque<state::ContractCall>& entry1, const std::deque<state::ContractCall>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		if (entry1.empty())
			return;

		for (auto i = 0u; i < entry1.size(); i++) {
			const auto& expectedContractCall = entry1[i];
			const auto& actualContractCall = entry2[i];
			EXPECT_EQ(expectedContractCall.FileName, actualContractCall.FileName);
			EXPECT_EQ(expectedContractCall.ActualArguments, actualContractCall.ActualArguments);
			EXPECT_EQ(expectedContractCall.BlockHeight, actualContractCall.BlockHeight);
			EXPECT_EQ(expectedContractCall.CallId, actualContractCall.CallId);
			EXPECT_EQ(expectedContractCall.Caller, actualContractCall.Caller);
			EXPECT_EQ(expectedContractCall.DownloadCallPayment, actualContractCall.DownloadCallPayment);
			EXPECT_EQ(expectedContractCall.ExecutionCallPayment, actualContractCall.ExecutionCallPayment);
			EXPECT_EQ(expectedContractCall.FunctionName, actualContractCall.FunctionName);
			AssertEqualServicePayments(expectedContractCall.ServicePayments, actualContractCall.ServicePayments);
		}
	}

	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.driveKey(), entry2.driveKey());
		EXPECT_EQ(entry1.executionPaymentKey(), entry2.executionPaymentKey());
		EXPECT_EQ(entry1.deploymentBaseModificationId(), entry2.deploymentBaseModificationId());
		EXPECT_EQ(entry1.assignee(), entry2.assignee());
		EXPECT_EQ(entry1.deploymentStatus(), entry2.deploymentStatus());
		EXPECT_EQ(entry1.nextBatchId(), entry2.nextBatchId());
		EXPECT_EQ(entry1.releasedTransactions(), entry2.releasedTransactions());
		AssertEqualBatches(entry1.batches(), entry2.batches());
		AssertEqualExecutorsInfo(entry1.executorsInfo(), entry2.executorsInfo());
		AssertEqualRequestedCalls(entry1.requestedCalls(), entry2.requestedCalls());
		AssertEqualAutomaticExecutionsInfo(entry1.automaticExecutionsInfo(), entry2.automaticExecutionsInfo());
	}

	void AssertEqualDriveContract(const state::DriveContractEntry& entry1, const state::DriveContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.contractKey(), entry2.contractKey());
	}

    void AddAccountState(
            cache::AccountStateCacheDelta& accountStateCache,
            const Key& publicKey,
            const Height& height,
            const std::vector<model::Mosaic>& mosaics){
        accountStateCache.addAccount(publicKey, height);
        auto accountStateIter = accountStateCache.find(publicKey);
        auto& accountState = accountStateIter.get();
        for (auto& mosaic : mosaics)
            accountState.Balances.credit(mosaic.MosaicId, mosaic.Amount);
    }

	uint16_t DriveStateBrowserImpl::getOrderedReplicatorsCount(const catapult::cache::ReadOnlyCatapultCache& cache, const catapult::Key& driveKey) const {
        const auto& driveCache = cache.template sub<cache::BcDriveCache>();
        auto driveIter = driveCache.find(driveKey);
        const auto& driveEntry = driveIter.get();
        return driveEntry.replicatorCount();
	}

    std::set<Key> DriveStateBrowserImpl::getReplicators(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
        const auto& driveCache = cache.template sub<cache::BcDriveCache>();
        auto driveIter = driveCache.find(driveKey);
        const auto& driveEntry = driveIter.get();
        return driveEntry.replicators();
    }

	std::set<Key> DriveStateBrowserImpl::getDrives(const cache::ReadOnlyCatapultCache &cache, const Key &replicatorKey) const {
		return {};
	}

	Hash256 DriveStateBrowserImpl::getDriveState(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
		return {};
	}

	Hash256 DriveStateBrowserImpl::getLastModificationId(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
		return {};
	}

    void LiquidityProviderExchangeObserverImpl::creditMosaics(
			observers::ObserverContext& context,
			const Key& currencyDebtor,
			const Key& mosaicCreditor,
			const UnresolvedMosaicId& unresolvedMosaicId,
			const UnresolvedAmount& unresolvedMosaicAmount) const {
		auto resolvedAmount = context.Resolvers.resolve(unresolvedMosaicAmount);
		creditMosaics(context, currencyDebtor, mosaicCreditor, unresolvedMosaicId, resolvedAmount);
	}

	void LiquidityProviderExchangeObserverImpl::debitMosaics(
			observers::ObserverContext& context,
			const Key& mosaicDebtor,
			const Key& currencyCreditor,
			const UnresolvedMosaicId& unresolvedMosaicId,
			const UnresolvedAmount& unresolvedMosaicAmount) const {
		auto resolvedAmount = context.Resolvers.resolve(unresolvedMosaicAmount);
		debitMosaics(context, mosaicDebtor, currencyCreditor, unresolvedMosaicId, resolvedAmount);
	}

	void LiquidityProviderExchangeObserverImpl::creditMosaics(
			observers::ObserverContext& context,
			const Key& currencyDebtor,
			const Key& mosaicCreditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto debtorAccountIter = accountStateCache.find(currencyDebtor);
		auto& debtorAccount = debtorAccountIter.get();
		auto creditorAccountIter = accountStateCache.find(mosaicCreditor);
		auto& creditorAccount = creditorAccountIter.get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		const auto resolvedMosaicId = MosaicId(mosaicId.unwrap());

		debtorAccount.Balances.debit(currencyMosaicId, mosaicAmount);
		creditorAccount.Balances.credit(resolvedMosaicId, mosaicAmount);
	}

	void LiquidityProviderExchangeObserverImpl::debitMosaics(
			observers::ObserverContext& context,
			const Key& mosaicDebtor,
			const Key& currencyCreditor,
			const UnresolvedMosaicId& mosaicId,
			const Amount& mosaicAmount) const {
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto debtorAccountIter = accountStateCache.find(mosaicDebtor);
		auto& debtorAccount = debtorAccountIter.get();
		auto creditorAccountIter = accountStateCache.find(currencyCreditor);
		auto& creditorAccount = creditorAccountIter.get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		const auto resolvedMosaicId = MosaicId(mosaicId.unwrap());

		debtorAccount.Balances.debit(resolvedMosaicId, mosaicAmount);
		creditorAccount.Balances.credit(currencyMosaicId, mosaicAmount);
	}
}}