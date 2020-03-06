/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/OperationIdentifyTransaction.h"
#include "src/model/StartOperationTransaction.h"
#include "src/model/EndOperationTransaction.h"
#include "src/model/OperationReceiptType.h"
#include "src/observers/Observers.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS AggregateTransactionHashObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(AggregateTransactionHash,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::OperationCacheFactory>;
		using Notification = model::AggregateTransactionHashNotification<1>;

		const auto Operation_Token = test::GenerateRandomByteArray<Hash256>();

		state::OperationEntry CreateOperationEntry(const std::vector<Hash256>& aggregateHashes) {
			state::OperationEntry entry(Operation_Token);
			entry.TransactionHashes = aggregateHashes;
			return entry;
		}

		void RunTest(
				NotifyMode mode,
				const Hash256& aggregateHash,
				const std::vector<Hash256>& initialAggregateHashes,
				const std::vector<Hash256>& expectedAggregateHashes,
				const test::TransactionBuffer& transactionBuffer) {
			// Arrange:
			ObserverTestContext context(mode, Height(124));
			Notification notification(aggregateHash, transactionBuffer.transactionCount(), transactionBuffer.transactions());
			auto pObserver = CreateAggregateTransactionHashObserver();
			auto& operationCache = context.cache().sub<cache::OperationCache>();

			// Populate cache.
			operationCache.insert(CreateOperationEntry(initialAggregateHashes));

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert:
			auto operationIter = operationCache.find(Operation_Token);
			const auto& actualEntry = operationIter.get();
			auto expectedEntry = CreateOperationEntry(expectedAggregateHashes);
			test::AssertEqualOperationData(expectedEntry, actualEntry);
		}

		struct OperationIdentifyTraits {
			static auto CreateTransaction() {
				return test::CreateOperationIdentifyTransaction<model::EmbeddedOperationIdentifyTransaction>();
			}
		};

		struct EndOperationTraits {
			static auto CreateTransaction() {
				return test::CreateEndOperationTransaction<model::EmbeddedEndOperationTransaction>(1);
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_OperationIdentify) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<OperationIdentifyTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_EndOperation) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<EndOperationTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TEST(TEST_CLASS, AggregateTransactionHash_Commit_NoSubTransactions) {
		std::vector<Hash256> initialAggregateHashes = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> expectedAggregateHashes = initialAggregateHashes;
		RunTest(
			NotifyMode::Commit,
			test::GenerateRandomByteArray<Hash256>(),
			initialAggregateHashes,
			expectedAggregateHashes,
			test::TransactionBuffer());
	}

	TEST(TEST_CLASS, AggregateTransactionHash_Commit_NoOperationEndOrIdentifyTransactions) {
		std::vector<Hash256> initialAggregateHashes = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> expectedAggregateHashes = initialAggregateHashes;
		test::TransactionBuffer buffer;
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));

		RunTest(
			NotifyMode::Commit,
			test::GenerateRandomByteArray<Hash256>(),
			initialAggregateHashes,
			expectedAggregateHashes,
			buffer);
	}

	TRAITS_BASED_TEST(AggregateTransactionHash_Commit_AddsTransactionHash) {
		auto pSubTransaction = TTraits::CreateTransaction();
		pSubTransaction->OperationToken = Operation_Token;
		auto aggregateHash = test::GenerateRandomByteArray<Hash256>();
		std::vector<Hash256> initialAggregateHashes = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> expectedAggregateHashes = initialAggregateHashes;
		expectedAggregateHashes.push_back(aggregateHash);
		test::TransactionBuffer buffer;
		buffer.addTransaction(pSubTransaction);
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));

		RunTest(
			NotifyMode::Commit,
			aggregateHash,
			initialAggregateHashes,
			expectedAggregateHashes,
			buffer);
	}

	TEST(TEST_CLASS, AggregateTransactionHash_Rollback_NoSubTransactions) {
		std::vector<Hash256> initialAggregateHashes = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> expectedAggregateHashes = initialAggregateHashes;

		RunTest(
			NotifyMode::Rollback,
			test::GenerateRandomByteArray<Hash256>(),
			initialAggregateHashes,
			expectedAggregateHashes,
			test::TransactionBuffer());
	}

	TEST(TEST_CLASS, AggregateTransactionHash_Rollback_NoOperationEndOrIdentifyTransactions) {
		std::vector<Hash256> initialAggregateHashes = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> expectedAggregateHashes = initialAggregateHashes;
		test::TransactionBuffer buffer;
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));

		RunTest(
			NotifyMode::Rollback,
			test::GenerateRandomByteArray<Hash256>(),
			initialAggregateHashes,
			expectedAggregateHashes,
			buffer);
	}

	TRAITS_BASED_TEST(AggregateTransactionHash_Rollback_RemovesLastTransactionHash) {
		auto pSubTransaction = TTraits::CreateTransaction();
		pSubTransaction->OperationToken = Operation_Token;
		auto aggregateHash = test::GenerateRandomByteArray<Hash256>();
		std::vector<Hash256> expectedAggregateHashes = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> initialAggregateHashes = expectedAggregateHashes;
		initialAggregateHashes.push_back(aggregateHash);
		test::TransactionBuffer buffer;
		buffer.addTransaction(pSubTransaction);
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));

		RunTest(
			NotifyMode::Rollback,
			aggregateHash,
			initialAggregateHashes,
			expectedAggregateHashes,
			buffer);
	}

	TRAITS_BASED_TEST(AggregateTransactionHash_Rollback_FailsToRemoveMismatchedTransactionHash) {
			auto pSubTransaction = TTraits::CreateTransaction();
		pSubTransaction->OperationToken = Operation_Token;
		auto aggregateHash = test::GenerateRandomByteArray<Hash256>();
		test::TransactionBuffer buffer;
		buffer.addTransaction(pSubTransaction);
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));
		buffer.addTransaction(test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(1, 1));

		EXPECT_THROW(
			RunTest(
				NotifyMode::Rollback,
				aggregateHash,
				{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() },
				{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() },
				buffer),
			catapult_runtime_error
		);
	}
}}
