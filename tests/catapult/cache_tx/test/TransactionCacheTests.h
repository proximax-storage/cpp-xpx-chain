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
#include "catapult/cache_tx/MemoryCacheOptions.h"
#include "catapult/model/TransactionFeeCalculator.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	/// A container of basic unknown transactions cache tests.
	template<typename TTraits>
	struct BasicUnknownTransactionsTests {
	private:
		using CacheType = typename TTraits::CacheType;

	public:
		/// Asserts that unknownTransactions returns nothing when cache is empty.
		static void AssertUnknownTransactionsReturnsNoTransactionsWhenCacheIsEmpty() {
			// Arrange:
			auto pTransactionFeeCalculator = std::make_shared<model::TransactionFeeCalculator>();
			CacheType cache(CreateDefaultOptions(), pTransactionFeeCalculator);

			// Act:
			auto unknownInfos = TTraits::GetUnknownTransactions(cache.view(), {});

			// Assert:
			EXPECT_TRUE(unknownInfos.empty());
		}

		/// Asserts that unknownTransactions returns all transactions when filter is empty.
		static void AssertUnknownTransactionsReturnsAllTransactionsWhenFilterIsEmpty() {
			// Arrange:
			auto pTransactionFeeCalculator = std::make_shared<model::TransactionFeeCalculator>();
			CacheType cache(CreateDefaultOptions(), pTransactionFeeCalculator);
			auto transactionInfos = CreateTransactionInfos(5);
			TTraits::AddAllToCache(cache, transactionInfos);

			// Act:
			auto unknownInfos = TTraits::GetUnknownTransactions(cache.view(), {});

			// Assert:
			EXPECT_EQ(5u, unknownInfos.size());
			TTraits::AssertUnknownResult(transactionInfos, unknownInfos);
		}

		/// Asserts that unknownTransactions returns all transactions not in filter.
		static void AssertUnknownTransactionsReturnsAllTransactionsNotInFilter() {
			// Arrange:
			auto pTransactionFeeCalculator = std::make_shared<model::TransactionFeeCalculator>();
			CacheType cache(CreateDefaultOptions(), pTransactionFeeCalculator);
			auto transactionInfos = CreateTransactionInfos(5);
			TTraits::AddAllToCache(cache, transactionInfos);

			// Act:
			auto unknownInfos = TTraits::GetUnknownTransactions(cache.view(), {
				TTraits::MapToFilterId(transactionInfos[1]),
				TTraits::MapToFilterId(transactionInfos[2]),
				TTraits::MapToFilterId(transactionInfos[4])
			});

			// Assert:
			EXPECT_EQ(2u, unknownInfos.size());
			decltype(transactionInfos) expectedInfos;
			expectedInfos.push_back(transactionInfos[0].copy());
			expectedInfos.push_back(transactionInfos[3].copy());
			TTraits::AssertUnknownResult(expectedInfos, unknownInfos);
		}

		/// Asserts that unknownTransactions returns nothing when all transactions in cache are known.
		static void AssertUnknownTransactionsReturnsNoTransactionsWhenAllTransactionsAreKnown() {
			// Arrange:
			auto pTransactionFeeCalculator = std::make_shared<model::TransactionFeeCalculator>();
			CacheType cache(CreateDefaultOptions(), pTransactionFeeCalculator);
			auto transactionInfos = CreateTransactionInfos(5);
			TTraits::AddAllToCache(cache, transactionInfos);

			// Act:
			auto unknownInfos = TTraits::GetUnknownTransactions(cache.view(), {
				TTraits::MapToFilterId(transactionInfos[0]),
				TTraits::MapToFilterId(transactionInfos[1]),
				TTraits::MapToFilterId(transactionInfos[2]),
				TTraits::MapToFilterId(transactionInfos[3]),
				TTraits::MapToFilterId(transactionInfos[4])
			});

			// Assert:
			EXPECT_TRUE(unknownInfos.empty());
		}

	private:
		static cache::MemoryCacheOptions CreateDefaultOptions() {
			return cache::MemoryCacheOptions(1'000'000, 1'000);
		}
	};

#define MAKE_UNKNOWN_TRANSACTIONS_TEST(TEST_CLASS, TRAITS_NAME, TEST_NAME) \
	TEST(TEST_CLASS, TEST_NAME) { test::BasicUnknownTransactionsTests<TRAITS_NAME>::Assert##TEST_NAME(); }

#define DEFINE_BASIC_UNKNOWN_TRANSACTIONS_TESTS(TEST_CLASS, TRAITS_NAME) \
	MAKE_UNKNOWN_TRANSACTIONS_TEST(TEST_CLASS, TRAITS_NAME, UnknownTransactionsReturnsNoTransactionsWhenCacheIsEmpty) \
	MAKE_UNKNOWN_TRANSACTIONS_TEST(TEST_CLASS, TRAITS_NAME, UnknownTransactionsReturnsAllTransactionsWhenFilterIsEmpty) \
	MAKE_UNKNOWN_TRANSACTIONS_TEST(TEST_CLASS, TRAITS_NAME, UnknownTransactionsReturnsAllTransactionsNotInFilter) \
	MAKE_UNKNOWN_TRANSACTIONS_TEST(TEST_CLASS, TRAITS_NAME, UnknownTransactionsReturnsNoTransactionsWhenAllTransactionsAreKnown)
}}
