/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/cache_core/AccountStateCache.h"
#include "plugins/txes/lock_shared/tests/test/LockInfoCacheTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

	/// Test suite for observers using LockStatusAccountBalanceObserver helper.
	template<typename TTraits>
	struct LockStatusObserverTests {
	private:
		static void AssertObserverSetsStatusToUsedAndCreditsBalanceOnCommit(Amount initialAmount) {
			// Arrange:
			auto lockInfo = TTraits::BasicTraits::CreateLockInfo();

			// Act:
			RunTest(
					typename TTraits::ObserverTestContext(NotifyMode::Commit, Height{444}),
					lockInfo,
					[&lockInfo, initialAmount](auto& cache, auto& accountState) {
						cache.insert(lockInfo);
						if (Amount(0) != initialAmount)
							accountState.Balances.credit(lockInfo.Mosaics.begin()->first, initialAmount, accountState.AddressHeight);
					},
					[&lockInfo, initialAmount](const auto& lockInfoCache, const auto& accountState, auto& observerContext) {
						// Assert: status and balance
						const auto& key = TTraits::BasicTraits::ToKey(lockInfo);
						auto resultIter = lockInfoCache.find(key);
						const auto& result = resultIter.get();
						EXPECT_EQ(state::LockStatus::Used, result.Status);
						auto expectedBalance = lockInfo.Mosaics.begin()->second + initialAmount;
						EXPECT_EQ(expectedBalance, accountState.Balances.get(result.Mosaics.begin()->first));

						auto pStatement = observerContext.statementBuilder().build();
						ASSERT_EQ(1u, pStatement->TransactionStatements.size());
						const auto& receiptPair = *pStatement->TransactionStatements.find(model::ReceiptSource());
						ASSERT_EQ(1u, receiptPair.second.size());

						const auto& receipt = static_cast<const model::BalanceChangeReceipt&>(receiptPair.second.receiptAt(0));
						ASSERT_EQ(sizeof(model::BalanceChangeReceipt), receipt.Size);
						EXPECT_EQ(1u, receipt.Version);
						EXPECT_EQ(TTraits::Receipt_Type, receipt.Type);
						EXPECT_EQ(accountState.PublicKey, receipt.Account);
						EXPECT_EQ(lockInfo.Mosaics.begin()->first, receipt.MosaicId);
						EXPECT_EQ(lockInfo.Mosaics.begin()->second, receipt.Amount);
					});
		}

	public:
		static void AssertObserverSetsStatusToUsedAndCreditsBalanceOnCommit() {
			AssertObserverSetsStatusToUsedAndCreditsBalanceOnCommit(Amount(0));
		}

		static void AssertObserverSetsStatusToUsedAndCreditsToExistingBalanceOnCommit() {
			AssertObserverSetsStatusToUsedAndCreditsBalanceOnCommit(Amount(100));
		}

		static void AssertObserverSetsStatusToUnusedAndDebitsBalanceOnRollback() {
			// Arrange:
			auto lockInfo = TTraits::BasicTraits::CreateLockInfo();
			lockInfo.Status = state::LockStatus::Used;

			// Act:
			RunTest(
					typename TTraits::ObserverTestContext(NotifyMode::Rollback, Height{444}),
					lockInfo,
					[&lockInfo](auto& cache, auto& accountState) {
						cache.insert(lockInfo);
						for (const auto& pair : lockInfo.Mosaics)
							accountState.Balances.credit(pair.first, pair.second + Amount(100), accountState.AddressHeight);
					},
					[&lockInfo](const auto& lockInfoCache, const auto& accountState, auto& observerContext) {
						// Assert: status and balance
						const auto& key = TTraits::BasicTraits::ToKey(lockInfo);
						auto resultIter = lockInfoCache.find(key);
						const auto& result = resultIter.get();
						EXPECT_EQ(state::LockStatus::Unused, result.Status);
						EXPECT_EQ(Amount(100), accountState.Balances.get(lockInfo.Mosaics.begin()->first));

						auto pStatement = observerContext.statementBuilder().build();
						ASSERT_EQ(0u, pStatement->TransactionStatements.size());
					});
		}

	private:
		template<typename TSeedCacheFunc, typename TCheckCacheFunc>
		static void RunTest(
				typename TTraits::ObserverTestContext&& context,
				const typename TTraits::BasicTraits::ValueType& lockInfo,
				TSeedCacheFunc seedCache,
				TCheckCacheFunc checkCache) {
			// Arrange:
			auto& accountStateCacheDelta = context.cache().template sub<cache::AccountStateCache>();
			auto accountId = TTraits::DestinationAccount(lockInfo);
			accountStateCacheDelta.addAccount(accountId, Height(1));
			auto& accountState = accountStateCacheDelta.find(accountId).get();

			auto& lockInfoCacheDelta = context.cache().template sub<typename TTraits::BasicTraits::CacheType>();
			seedCache(lockInfoCacheDelta, accountState);

			auto pObserver = TTraits::CreateObserver();

			// Act:
			typename TTraits::NotificationBuilder notificationBuilder;
			notificationBuilder.prepare(lockInfo);

			auto notification = notificationBuilder.notification();
			test::ObserveNotification(*pObserver, notification, context);

			// Assert
			checkCache(lockInfoCacheDelta, accountState, context);
		}
	};
}}


#define MAKE_LOCK_STATUS_OBSERVER_TEST_EXT(TRAITS_NAME, TEST_NAME, SUFFIX) \
	TEST(TEST_CLASS, TEST_NAME##SUFFIX) { LockStatusObserverTests<TRAITS_NAME>::Assert##TEST_NAME(); }

#define MAKE_LOCK_STATUS_OBSERVER_TEST(TRAITS_NAME, TEST_NAME) \
	MAKE_LOCK_STATUS_OBSERVER_TEST_EXT(TRAITS_NAME, TEST_NAME, )

#define DEFINE_LOCK_STATUS_OBSERVER_TESTS_EXT(TRAITS_NAME, SUFFIX) \
	MAKE_LOCK_STATUS_OBSERVER_TEST_EXT(TRAITS_NAME, ObserverSetsStatusToUsedAndCreditsBalanceOnCommit, SUFFIX) \
	MAKE_LOCK_STATUS_OBSERVER_TEST_EXT(TRAITS_NAME, ObserverSetsStatusToUsedAndCreditsToExistingBalanceOnCommit, SUFFIX) \
	MAKE_LOCK_STATUS_OBSERVER_TEST_EXT(TRAITS_NAME, ObserverSetsStatusToUnusedAndDebitsBalanceOnRollback, SUFFIX)

#define DEFINE_LOCK_STATUS_OBSERVER_TESTS(TRAITS_NAME) \
	DEFINE_LOCK_STATUS_OBSERVER_TESTS_EXT(TRAITS_NAME, )


