/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/OperationReceiptType.h"
#include "src/observers/Observers.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS EndOperationObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndOperation,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::OperationCacheFactory>;
		using Notification = model::EndOperationNotification<1>;

		const auto Operation_Token = test::GenerateRandomByteArray<Hash256>();
		const auto Initiator = test::GenerateRandomByteArray<Key>();
		const model::OperationResult Operation_Result = 1234;

		const std::vector<model::UnresolvedMosaic> Spent_Mosaics{
			{ test::UnresolveXor(MosaicId(1)), Amount(20) },
			{ test::UnresolveXor(MosaicId(2)), Amount(10) },
		};
		const auto Num_Spent_Mosaics = Spent_Mosaics.size();
		const std::vector<std::map<MosaicId, Amount>> Locked_Mosaics{
			{
				{ MosaicId(1), Amount(20) },
				{ MosaicId(2), Amount(20) },
				{ MosaicId(3), Amount(20) },
			},
			{
				{ MosaicId(2), Amount(10) },
				{ MosaicId(3), Amount(20) },
			}
		};
		const std::vector<std::vector<model::Mosaic>> Initiator_Balances{
			{
				{ MosaicId(1), Amount(100) },
				{ MosaicId(2), Amount(100) },
				{ MosaicId(3), Amount(100) },
			},
			{
				{ MosaicId(1), Amount(100) },
				{ MosaicId(2), Amount(110) },
				{ MosaicId(3), Amount(120) },
			}
		};

		state::OperationEntry CreateOperationEntry(model::OperationResult result, state::LockStatus status, const std::map<MosaicId, Amount>& mosaics) {
			state::OperationEntry entry(Operation_Token);
			entry.Account = Initiator;
			entry.Result = result;
			entry.Status = status;
			entry.Mosaics = mosaics;
			return entry;
		}
	}

	TEST(TEST_CLASS, EndOperation_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Height(124));
		Notification notification(test::GenerateRandomByteArray<Key>(), Operation_Token, Spent_Mosaics.data(), Spent_Mosaics.size(), Operation_Result);
		auto pObserver = CreateEndOperationObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(model::Operation_Result_None, state::LockStatus::Unused, Locked_Mosaics[0]));
		test::SetCacheBalances(context.cache(), Initiator, Initiator_Balances[0]);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert:
		auto operationIter = operationCache.find(Operation_Token);
		const auto& actualEntry = operationIter.get();
		auto expectedEntry = CreateOperationEntry(Operation_Result, state::LockStatus::Used, Locked_Mosaics[1]);
		test::AssertEqualOperationData(expectedEntry, actualEntry);

		test::AssertBalances(context.cache(), Initiator, Initiator_Balances[1]);

		auto pStatement = context.statementBuilder().build();
		ASSERT_EQ(1u, pStatement->TransactionStatements.size());
		const auto& receiptPair = *pStatement->TransactionStatements.find(model::ReceiptSource());
		ASSERT_EQ(Num_Spent_Mosaics, receiptPair.second.size());

		for (auto i = 0u; i < Num_Spent_Mosaics; ++i) {
			const auto &receipt = static_cast<const model::BalanceChangeReceipt &>(receiptPair.second.receiptAt(i));
			ASSERT_EQ(sizeof(model::BalanceChangeReceipt), receipt.Size);
			EXPECT_EQ(1u, receipt.Version);
			EXPECT_EQ(model::Receipt_Type_Operation_Ended, receipt.Type);
			EXPECT_EQ(Initiator, receipt.Account);
			EXPECT_EQ(MosaicId(i + 2), receipt.MosaicId);
			EXPECT_EQ(Amount((i + 1) * 10), receipt.Amount);
		}
	}

	TEST(TEST_CLASS, EndOperation_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Height(124));
		Notification notification(test::GenerateRandomByteArray<Key>(), Operation_Token, Spent_Mosaics.data(), Spent_Mosaics.size(), Operation_Result);
		auto pObserver = CreateEndOperationObserver();
		auto& operationCache = context.cache().sub<cache::OperationCache>();

		// Populate cache.
		operationCache.insert(CreateOperationEntry(Operation_Result, state::LockStatus::Used, Locked_Mosaics[1]));
		test::SetCacheBalances(context.cache(), Initiator, Initiator_Balances[1]);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert:
		auto operationIter = operationCache.find(Operation_Token);
		const auto& actualEntry = operationIter.get();
		auto expectedEntry = CreateOperationEntry(model::Operation_Result_None, state::LockStatus::Unused, Locked_Mosaics[0]);
		test::AssertEqualOperationData(expectedEntry, actualEntry);

		test::AssertBalances(context.cache(), Initiator, Initiator_Balances[0]);

		auto pStatement = context.statementBuilder().build();
		ASSERT_EQ(0u, pStatement->TransactionStatements.size());
	}
}}
